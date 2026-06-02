// Stripped-binary IL2CPP API recovery for ARMv7 (A32): BL histogram → thunk → forward-decode window.
#pragma once

#if defined(__arm__)

#include <algorithm>
#include <climits>
#include <cstdint>
#include <cstring>

#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Logger.h"
#include "API/detail/arm32_decode.hpp"
#include "API/detail/common.hpp"
#include "API/detail/elf_view.hpp"
#include "API/detail/proc_maps.hpp"
#include "API/detail/safe_read.hpp"

#include "xdl.h"

namespace il2cpp_api {
namespace detail {

constexpr int k_il2_cpp_cstr_scan_max_arm32 = 160;

/** Half-open [scan_lo, scan_hi) inside `seg`; empty range returns false. */
inline bool scanner_region_text_window(const MappedRegion &seg, bool text_bounds_ok, uintptr_t tva,
                                       uintptr_t tve, uintptr_t *scan_lo, uintptr_t *scan_hi) {
    if (!scan_lo || !scan_hi) return false;
    if (!text_bounds_ok) {
        *scan_lo = seg.va_start;
        *scan_hi = seg.va_end;
    } else {
        if (seg.va_end <= tva || seg.va_start >= tve) return false;
        *scan_lo = std::max(seg.va_start, tva);
        *scan_hi = std::min(seg.va_end, tve);
    }
    return *scan_lo + UINTPTR_C(4) <= *scan_hi;
}

inline uint32_t u32_load_le_buf_arm32(const unsigned char *p) {
    return static_cast<uint32_t>(static_cast<unsigned>(p[0])) |
           (static_cast<uint32_t>(static_cast<unsigned>(p[1])) << 8) |
           (static_cast<uint32_t>(static_cast<unsigned>(p[2])) << 16) |
           (static_cast<uint32_t>(static_cast<unsigned>(p[3])) << 24);
}

inline bool read_printable_bounded_cstr_arm32(uintptr_t va, const MappedRegion *segs, int nseg,
                                              std::string *out) {
    unsigned char buf[k_il2_cpp_cstr_scan_max_arm32];
    if (!memcpy_from_regions(segs, nseg, va, buf, sizeof(buf))) return false;

    size_t len = 0;
    while (len < sizeof(buf) && buf[len] != '\0') {
        unsigned char c = buf[len];
        if (c < 0x20 || c > 0x7e) return false;
        ++len;
    }
    if (len == 0) return false;
    out->assign(reinterpret_cast<const char *>(buf), len);
    return true;
}

/** Match tools/memdump_so.c::pick_fn_va_rx: prefer max candidate in RX; else max aligned (VMA). */
inline uintptr_t pick_fn_va_rx_arm32(const std::vector<uintptr_t> &fpool, uintptr_t reg_tgt_va,
                                     const MappedRegion *segs, int nseg) {
    uintptr_t best_rx = 0;
    uintptr_t best_any = 0;
    for (uintptr_t a : fpool) {
        if (a == reg_tgt_va) continue;
        if ((a & 3u) != 0) continue;
        if (a > best_any) best_any = a;
        if (addr_in_any_region(segs, nseg, a, true) && a > best_rx) best_rx = a;
    }
    return best_rx ? best_rx : best_any;
}

inline uintptr_t pick_registration_thunk_arm32(const std::unordered_map<uint64_t, int> &counts,
                                               bool elf_ok, uintptr_t load_base,
                                               const ElfLoadSlice *loads, int nloads,
                                               const MappedRegion *regions, int nregs) {
    constexpr int k_expect = 248;
    constexpr int k_maxd = 40;

    int best_rank = INT_MIN;
    uintptr_t best_va = 0;

    for (const auto &kv : counts) {
        const uintptr_t tgt = static_cast<uintptr_t>(kv.first);
        const int cnt = kv.second;
        const int d = cnt > k_expect ? cnt - k_expect : k_expect - cnt;
        if (d > k_maxd || cnt < 60) continue;

        uintptr_t fo = 0;
        const bool fo_ok =
            va_to_hdr_foff(elf_ok, tgt, load_base, loads, nloads, regions, nregs, &fo);
        const bool in_band = fo_ok && fo >= static_cast<uintptr_t>(0x3330000) &&
                             fo <= static_cast<uintptr_t>(0x33c0000);
        const int rank = static_cast<int>(in_band ? 100000 : 0) - d * 100 + cnt;
        if (rank > best_rank) {
            best_rank = rank;
            best_va = tgt;
        }
    }

    if (best_va != 0) return best_va;

    int max_cnt = -1;
    uintptr_t max_va = 0;
    for (const auto &kv : counts) {
        if (kv.second > max_cnt) {
            max_cnt = kv.second;
            max_va = static_cast<uintptr_t>(kv.first);
        }
    }
    return max_va;
}

inline void merge_mov_pair_arm32(unsigned rd, uint16_t lo, uint16_t hi, uint64_t *regv,
                                 unsigned char *valid, uint8_t *have_lo, uint8_t *have_hi) {
    if (rd >= 16 || !regv || !valid || !have_lo || !have_hi) return;
    const uint32_t v = (static_cast<uint32_t>(hi) << 16) | static_cast<uint32_t>(lo);
    regv[rd] = static_cast<uint64_t>(v);
    valid[rd] = 1;
    have_lo[rd] = 0;
    have_hi[rd] = 0;
}

inline bool forward_track_window_arm32(uintptr_t start_pc, uintptr_t bl_pc,
                                       const MappedRegion *all_regs, int n_all, uint64_t *regv,
                                       unsigned char *valid) {
    const uintptr_t span = bl_pc - start_pc;
    if (!regv || !valid || start_pc >= bl_pc || (span % 4) != 0) return false;

    std::memset(valid, 0, 16);
    std::memset(regv, 0, 16 * sizeof(uint64_t));

    uint16_t lo_half[16] = {};
    uint16_t hi_half[16] = {};
    uint8_t have_lo[16] = {};
    uint8_t have_hi[16] = {};

    std::vector<uint8_t> w(span);
    if (!memcpy_from_regions(all_regs, n_all, start_pc, w.data(), span)) return false;

    const size_t nins = span / 4;
    for (size_t j = 0; j < nins; ++j) {
        const uintptr_t ipc = start_pc + j * 4;
        const uint32_t val = u32_load_le_buf_arm32(w.data() + j * 4);

        unsigned rt = 0;
        unsigned rd = 0;
        unsigned rm = 0;
        uint32_t addr = 0;
        uint16_t imm16 = 0;

        if (decode_ldr_literal(static_cast<uint32_t>(ipc), val, &rt, &addr)) {
            if (rt < 16) {
                uint32_t loaded = 0;
                if (memcpy_from_regions(all_regs, n_all, addr, &loaded, sizeof(loaded))) {
                    regv[rt] = static_cast<uint64_t>(loaded);
                    valid[rt] = 1;
                    have_lo[rt] = 0;
                    have_hi[rt] = 0;
                }
            }
            continue;
        }
        if (decode_add_pc_imm(static_cast<uint32_t>(ipc), val, &rd, &addr)) {
            if (rd < 16) {
                regv[rd] = static_cast<uint64_t>(addr);
                valid[rd] = 1;
                have_lo[rd] = 0;
                have_hi[rd] = 0;
            }
            continue;
        }
        if (decode_add_pc_reg(val, &rd, &rm)) {
            if (rd < 16 && rm < 16 && valid[rm]) {
                const uint32_t sum = static_cast<uint32_t>(ipc) + 8u +
                                     static_cast<uint32_t>(regv[rm]);
                regv[rd] = static_cast<uint64_t>(sum);
                valid[rd] = 1;
                have_lo[rd] = 0;
                have_hi[rd] = 0;
            }
            continue;
        }
        if (decode_movw(val, &rd, &imm16)) {
            if (rd < 16) {
                lo_half[rd] = imm16;
                have_lo[rd] = 1;
                if (have_hi[rd])
                    merge_mov_pair_arm32(rd, lo_half[rd], hi_half[rd], regv, valid, have_lo,
                                         have_hi);
            }
            continue;
        }
        if (decode_movt(val, &rd, &imm16)) {
            if (rd < 16) {
                hi_half[rd] = imm16;
                have_hi[rd] = 1;
                if (have_lo[rd])
                    merge_mov_pair_arm32(rd, lo_half[rd], hi_half[rd], regv, valid, have_lo,
                                         have_hi);
            }
            continue;
        }
        int32_t ldr_off = 0;
        if (decode_ldr_imm(val, &rt, &rd, &ldr_off)) {
            if (rt < 16 && rd < 16 && valid[rd]) {
                const uint32_t base = static_cast<uint32_t>(regv[rd]);
                const uint32_t target =
                    static_cast<uint32_t>(static_cast<int64_t>(base) +
                                          static_cast<int64_t>(ldr_off));
                uint32_t loaded = 0;
                if (memcpy_from_regions(all_regs, n_all, target, &loaded, sizeof(loaded))) {
                    regv[rt] = static_cast<uint64_t>(loaded);
                    valid[rt] = 1;
                    have_lo[rt] = 0;
                    have_hi[rt] = 0;
                }
            }
        }
    }
    return true;
}

inline void analyze_bl_to_thunk_arm32(uintptr_t bl_pc, uintptr_t seg_lo, uintptr_t seg_hi,
                                      uintptr_t thunk_va, const MappedRegion *allregs, int n_all,
                                      uintptr_t vma_lo, uintptr_t vma_hi, ApiList *raw_out,
                                      std::set<std::pair<std::string, uintptr_t>> *pair_seen,
                                      size_t site_idx, size_t n_bl_sites, size_t *window_scans_out,
                                      bool log_detail) {
    const uintptr_t low_bound =
        bl_pc >= seg_lo + static_cast<uintptr_t>(48) * 4 ? bl_pc - static_cast<uintptr_t>(48) * 4
                                                         : seg_lo;

    uintptr_t cur = bl_pc >= 4 ? bl_pc - 4 : UINTPTR_C(0);
    uintptr_t start_pc = low_bound;
    int back_steps = 0;

    while (cur >= low_bound && cur >= seg_lo && back_steps < 48) {
        uint32_t w = 0;
        if (!safe_read_va(cur, &w, sizeof(w))) break;
        if (is_control_flow_arm(w)) {
            const uintptr_t next = cur + UINTPTR_C(4);
            start_pc = next > low_bound ? next : low_bound;
            break;
        }
        if (cur < UINTPTR_C(4)) break;
        cur -= UINTPTR_C(4);
        back_steps++;
    }

    if (start_pc + UINTPTR_C(4) > bl_pc) {
        if (log_detail) {
            SDKLOGW("[scanner] pass2 bl=0x%llx site=%zu/%zu: window too short (start_pc=0x%llx)",
                  JLOG_HEX(bl_pc), site_idx, n_bl_sites, JLOG_HEX(start_pc));
        }
        return;
    }

    if (window_scans_out) ++*window_scans_out;

    uint64_t regv[16] = {};
    unsigned char vld[16] = {};
    if (!forward_track_window_arm32(start_pc, bl_pc, allregs, n_all, regv, vld)) {
        if (log_detail) {
            SDKLOGW("[scanner] pass2 bl=0x%llx site=%zu/%zu: forward_track_window failed",
                  JLOG_HEX(bl_pc), site_idx, n_bl_sites);
        }
        return;
    }

    if (log_detail && site_idx < 5u) {
        for (unsigned r = 0; r < 4u; ++r) {
            if (!vld[r]) continue;
            SDKLOGI("[scanner-dbg] site=%zu r%u=0x%llx valid=1", site_idx, r, JLOG_HEX(regv[r]));
        }
    }

    std::string best_name;
    std::vector<uintptr_t> fpool;

    for (int r = 0; r < 16; ++r) {
        if (!vld[r]) continue;
        const uintptr_t p = static_cast<uintptr_t>(regv[r]);
        if (p < vma_lo || p >= vma_hi) continue;

        std::string nm;
        if (read_printable_bounded_cstr_arm32(p, allregs, n_all, &nm) && is_valid_api_name(nm)) {
            if (nm.size() > best_name.size()) best_name.swap(nm);
            continue;
        }
        if (addr_in_any_region(allregs, n_all, p, false) &&
            (p & UINTPTR_C(3)) == 0 && p != thunk_va) {
            fpool.push_back(p);
        }
    }

    if (best_name.empty()) {
        if (log_detail) {
            SDKLOGW("[scanner] pass2 bl=0x%llx site=%zu/%zu: no il2cpp API string in regs",
                  JLOG_HEX(bl_pc), site_idx, n_bl_sites);
        }
        return;
    }

    const uintptr_t fn_va = pick_fn_va_rx_arm32(fpool, thunk_va, allregs, n_all);
    if (fn_va == 0) {
        if (log_detail) {
            SDKLOGW("[scanner] pass2 bl=0x%llx site=%zu/%zu: fn_va pick failed (fpool=%zu)",
                  JLOG_HEX(bl_pc), site_idx, n_bl_sites, fpool.size());
        }
        return;
    }
    if (!addr_in_any_region(allregs, n_all, fn_va, false)) {
        if (log_detail) {
            SDKLOGW("[scanner] pass2 bl=0x%llx site=%zu/%zu: fn_va=0x%llx not in libil2cpp VMA",
                  JLOG_HEX(bl_pc), site_idx, n_bl_sites, JLOG_HEX(fn_va));
        }
        return;
    }

    if (!pair_seen->insert(std::make_pair(best_name, fn_va)).second) {
        if (log_detail) {
            SDKLOGW("[scanner] pass2 bl=0x%llx site=%zu/%zu: duplicate pair %s",
                  JLOG_HEX(bl_pc), site_idx, n_bl_sites, best_name.c_str());
        }
        return;
    }
    raw_out->push_back(std::make_pair(best_name, fn_va));
}

inline void pass2_collect_calls_thunk_arm32(uintptr_t thunk_va, const MappedRegion *regions,
                                            int n_regs, uintptr_t vma_lo, uintptr_t vma_hi,
                                            ApiList *raw_pairs, bool text_bounds_ok,
                                            uintptr_t text_va_start, uintptr_t text_va_end) {
    std::set<std::pair<std::string, uintptr_t>> pair_seen;
    raw_pairs->clear();

    size_t n_bl_thunk = 0;
    for (int si = 0; si < n_regs; ++si) {
        const MappedRegion &seg = regions[si];
        uintptr_t wlo = 0;
        uintptr_t whi = 0;
        if (!scanner_region_text_window(seg, text_bounds_ok, text_va_start, text_va_end, &wlo, &whi))
            continue;
        for (uintptr_t addr = wlo; addr + UINTPTR_C(4) <= whi; addr += UINTPTR_C(4)) {
            const uint32_t w = *reinterpret_cast<const uint32_t *>(addr);
            uint64_t tgt_u = 0;
            if (!decode_bl_arm(static_cast<uint32_t>(addr), w, &tgt_u)) continue;
            if (static_cast<uintptr_t>(tgt_u) != thunk_va) continue;
            ++n_bl_thunk;
        }
    }

    size_t site_idx = 0;
    size_t window_scans = 0;
    for (int si = 0; si < n_regs; ++si) {
        const MappedRegion &seg = regions[si];
        uintptr_t wlo = 0;
        uintptr_t whi = 0;
        if (!scanner_region_text_window(seg, text_bounds_ok, text_va_start, text_va_end, &wlo, &whi))
            continue;

        const uintptr_t seg_lo = text_bounds_ok ? wlo : seg.va_start;
        const uintptr_t seg_hi = text_bounds_ok ? whi : seg.va_end;

        for (uintptr_t addr = wlo; addr + UINTPTR_C(4) <= whi; addr += UINTPTR_C(4)) {
            const uint32_t w = *reinterpret_cast<const uint32_t *>(addr);
            uint64_t tgt_u = 0;
            if (!decode_bl_arm(static_cast<uint32_t>(addr), w, &tgt_u)) continue;
            if (static_cast<uintptr_t>(tgt_u) != thunk_va) continue;

            const bool log_detail = (site_idx < 5u) ||
                                    (n_bl_thunk > 0u && site_idx + 1u == n_bl_thunk) ||
                                    (site_idx != 0u && (site_idx % 512u == 0u));

            analyze_bl_to_thunk_arm32(addr, seg_lo, seg_hi, thunk_va, regions, n_regs,
                                      vma_lo, vma_hi, raw_pairs, &pair_seen, site_idx, n_bl_thunk,
                                      &window_scans, log_detail);
            ++site_idx;
        }
    }

    SDKLOGI("[scanner] pass2 bl_to_thunk_sites=%zu window_scans=%zu pairs=%zu", n_bl_thunk,
          window_scans, raw_pairs->size());
    const size_t n_show = std::min<size_t>(3, raw_pairs->size());
    for (size_t pi = 0; pi < n_show; ++pi) {
        SDKLOGI("[scanner] pass2 sample pair[%zu] %s -> 0x%llx", pi,
              raw_pairs->at(pi).first.c_str(), JLOG_HEX(raw_pairs->at(pi).second));
    }
}

inline ApiList resolve_stripped_arm32_manual(void *il2cpp_handle, uintptr_t il2cpp_base_param) {
    ApiList raw;
    if (!il2cpp_handle) {
        SDKLOGW("[scanner] fail: null il2cpp handle");
        return raw;
    }

    uintptr_t il2cpp_base = il2cpp_base_param;
    if (il2cpp_base == 0) {
        xdl_info_t info{};
        if (xdl_info(il2cpp_handle, XDL_DI_DLINFO, &info) != 0) {
            SDKLOGW("[scanner] fail: xdl_info for base");
            return raw;
        }
        il2cpp_base = reinterpret_cast<uintptr_t>(info.dli_fbase);
    }
    if (il2cpp_base == 0) {
        SDKLOGW("[scanner] fail: il2cpp_base=0");
        return raw;
    }

    SDKLOGI("[scanner] start base=0x%llx", JLOG_HEX(il2cpp_base));

    std::vector<MappedRegion> regions;
    if (!read_libil2cpp_regions(regions) || regions.empty()) {
        SDKLOGE("[scanner] no libil2cpp.so regions in maps");
        return raw;
    }

    SDKLOGI("[scanner] regions count=%zu", regions.size());
    for (size_t ri = 0; ri < regions.size(); ++ri) {
        const MappedRegion &g = regions[ri];
        SDKLOGI("[scanner]  region[%zu] va=[0x%llx,0x%llx) foff=0x%llx exec=%d len=%zu", ri,
              JLOG_HEX(g.va_start), JLOG_HEX(g.va_end), JLOG_HEX(g.file_off), g.is_exec ? 1 : 0,
              g.len);
    }

    ElfLoadSlice loads[kMaxPtLoadSlices] = {};
    int nloads = 0;
    uintptr_t load_base = 0;
    const int bias_rc = compute_load_bias(regions.data(), static_cast<int>(regions.size()),
                                          &load_base, loads, &nloads);
    const bool elf_ok = (bias_rc == 0);
    if (elf_ok) {
        SDKLOGI("[scanner] compute_load_bias ok load_base=0x%llx n_pt_load=%d", JLOG_HEX(load_base),
              nloads);
    } else {
        SDKLOGW("[scanner] compute_load_bias failed (rc=%d) n_pt_load=%d load_base=0x%llx", bias_rc,
              nloads, JLOG_HEX(load_base));
    }

    int n_exec = 0;
    size_t readable_bytes_total = 0;
    for (const MappedRegion &r : regions) {
        readable_bytes_total += r.len;
        if (r.is_exec) ++n_exec;
    }
    const int nr = static_cast<int>(regions.size());

    SDKLOGI("[scanner] readable_regions=%d exec_regions=%d (exec=0 OK, scanning all readable)", nr,
          n_exec);

    uintptr_t vma_lo = 0;
    uintptr_t vma_hi = 0;
    if (!vma_span_lo_hi_vec(regions, &vma_lo, &vma_hi)) {
        SDKLOGW("[scanner] fail: vma_span_lo_hi_vec");
        return raw;
    }

    DynsymCandidate dyn_cand{};
    const bool dynsym_anchor_ok =
        elf_ok &&
        find_registration_func_from_dynsym(regions.data(), nr, load_base, &dyn_cand);

    std::unordered_map<uint64_t, int> bl_counts;
    bl_counts.reserve(4096);

    uintptr_t tva = 0;
    uintptr_t tve = 0;
    bool narrow_scan_bounds = false;

    uintptr_t thunk_va = 0;
    int thunk_cnt = 0;

    if (dynsym_anchor_ok) {
        narrow_scan_bounds = true;
        tva = dyn_cand.va_start;
        tve = dyn_cand.va_end;
        SDKLOGI("[scanner] dynsym anchor: func va=[0x%llx,0x%llx) size=%zu KB thunk=0x%llx count=%d",
              JLOG_HEX(tva), JLOG_HEX(tve), static_cast<size_t>(tve - tva) / 1024u,
              JLOG_HEX(dyn_cand.thunk_va), dyn_cand.bl_to_thunk_count);

        thunk_va = dyn_cand.thunk_va;
        thunk_cnt = dyn_cand.bl_to_thunk_count;

        uintptr_t thunk_fo_fast = 0;
        const bool fo_ok_fast =
            va_to_hdr_foff(elf_ok, thunk_va, load_base, elf_ok ? loads : nullptr,
                           elf_ok ? nloads : 0, regions.data(), nr, &thunk_fo_fast);
        const bool in_band_fast =
            fo_ok_fast && thunk_fo_fast >= static_cast<uintptr_t>(0x3330000) &&
            thunk_fo_fast <= static_cast<uintptr_t>(0x33c0000);
        SDKLOGI("[scanner] thunk va=0x%llx file_off=0x%llx bl_count=%d in_api_band=%d fo_ok=%d "
              "(dynsym anchor fast path)",
              JLOG_HEX(thunk_va), JLOG_HEX(thunk_fo_fast), thunk_cnt, in_band_fast ? 1 : 0,
              fo_ok_fast ? 1 : 0);

        if (thunk_va == 0) {
            SDKLOGW("[scanner] pass1 registration thunk_va=0 (dynsym anchor invalid)");
            return raw;
        }

        pass2_collect_calls_thunk_arm32(thunk_va, regions.data(), nr, vma_lo, vma_hi, &raw,
                                        narrow_scan_bounds, tva, tve);

        if (!raw.empty()) {
            SDKLOGI("[scanner] done merged_pairs=%zu", raw.size());
            return raw;
        }

        SDKLOGW("[scanner] dynsym anchor produced 0 pairs, falling back to pass1 byte scan");
        bl_counts.clear();
    } else if (elf_ok) {
        SDKLOGW("[scanner] dynsym anchor not found, falling back to pass1 bounds");
    }

    narrow_scan_bounds = find_text_section_bounds(regions.data(), nr, &tva, &tve);
    if (narrow_scan_bounds) {
        SDKLOGI("[scanner] text section va=[0x%llx,0x%llx) size=%zu KB", JLOG_HEX(tva), JLOG_HEX(tve),
              static_cast<size_t>(tve - tva) / 1024u);
    } else if (elf_ok && find_pt_load_x_bounds(load_base, loads, nloads, &tva, &tve)) {
        narrow_scan_bounds = true;
        SDKLOGI("[scanner] using PT_LOAD PF_X bounds va=[0x%llx,0x%llx) size=%zu KB", JLOG_HEX(tva),
              JLOG_HEX(tve), static_cast<size_t>(tve - tva) / 1024u);
    } else {
        SDKLOGW("[scanner] no text/PT_LOAD-X bounds, scanning all regions");
    }

    size_t pass1_in_text_bytes = 0;
    for (int si = 0; si < nr; ++si) {
        const MappedRegion &seg = regions[static_cast<size_t>(si)];
        uintptr_t wlo = 0;
        uintptr_t whi = 0;
        if (!scanner_region_text_window(seg, narrow_scan_bounds, tva, tve, &wlo, &whi)) continue;
        pass1_in_text_bytes += static_cast<size_t>(whi - wlo);
        for (uintptr_t addr = wlo; addr + UINTPTR_C(4) <= whi; addr += UINTPTR_C(4)) {
            const uint32_t w = *reinterpret_cast<const uint32_t *>(addr);
            uint64_t tg = UINT64_C(0);
            if (!decode_bl_arm(static_cast<uint32_t>(addr), w, &tg)) continue;
            bl_counts[tg]++;
        }
    }

    const size_t pass1_skipped_mb =
        readable_bytes_total > pass1_in_text_bytes
            ? (readable_bytes_total - pass1_in_text_bytes) / (1024u * 1024u)
            : 0;
    SDKLOGI("[scanner] pass1 in_text_bytes=%zu (skipped %zu MB)", pass1_in_text_bytes, pass1_skipped_mb);

    if (bl_counts.empty()) {
        SDKLOGW("[scanner] pass1 no BL branch targets (empty histogram)");
        return raw;
    }

    std::vector<std::pair<uint64_t, int>> hist_sorted;
    hist_sorted.reserve(bl_counts.size());
    for (const auto &kv : bl_counts) hist_sorted.push_back(kv);
    std::sort(hist_sorted.begin(), hist_sorted.end(),
              [](const std::pair<uint64_t, int> &a, const std::pair<uint64_t, int> &b) {
                  if (a.second != b.second) return a.second > b.second;
                  return a.first < b.first;
              });
    const size_t nuniq = bl_counts.size();
    SDKLOGI("[scanner] pass1 bl_targets_unique=%zu", nuniq);
    const size_t ntop = std::min<size_t>(5, hist_sorted.size());
    for (size_t ti = 0; ti < ntop; ++ti) {
        SDKLOGI("[scanner] pass1 top%zu tgt=0x%llx count=%d", ti + 1u,
              JLOG_HEX(hist_sorted[ti].first), hist_sorted[ti].second);
    }

    thunk_va =
        pick_registration_thunk_arm32(bl_counts, elf_ok, load_base, elf_ok ? loads : nullptr,
                                       elf_ok ? nloads : 0, regions.data(), nr);

    const auto it_hit = bl_counts.find(static_cast<uint64_t>(thunk_va));
    thunk_cnt = (it_hit != bl_counts.end()) ? it_hit->second : 0;

    uintptr_t thunk_fo = 0;
    const bool fo_ok =
        va_to_hdr_foff(elf_ok, thunk_va, load_base, elf_ok ? loads : nullptr,
                       elf_ok ? nloads : 0, regions.data(), nr, &thunk_fo);
    const bool in_band = fo_ok && thunk_fo >= static_cast<uintptr_t>(0x3330000) &&
                         thunk_fo <= static_cast<uintptr_t>(0x33c0000);

    SDKLOGI("[scanner] thunk va=0x%llx file_off=0x%llx bl_count=%d in_api_band=%d fo_ok=%d",
          JLOG_HEX(thunk_va), JLOG_HEX(thunk_fo), thunk_cnt, in_band ? 1 : 0, fo_ok ? 1 : 0);

    if (thunk_va == 0) {
        SDKLOGW("[scanner] pass1 registration thunk_va=0 (picker found no candidate)");
        return raw;
    }

    pass2_collect_calls_thunk_arm32(thunk_va, regions.data(), nr, vma_lo, vma_hi, &raw,
                                    narrow_scan_bounds, tva, tve);

    SDKLOGI("[scanner] done merged_pairs=%zu", raw.size());

    return raw;
}

}  // namespace detail
}  // namespace il2cpp_api

#endif  // __arm__
