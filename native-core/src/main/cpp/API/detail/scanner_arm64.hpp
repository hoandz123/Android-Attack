// Stripped-binary IL2CPP API recovery for AArch64 (logic ported from legacy IL2CPP_API.hpp collect_il2cpp_api).
#pragma once

#if defined(__aarch64__)

#include <algorithm>
#include <climits>
#include <cstdint>
#include <cstring>
#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

#include "Logger.h"
#include "API/detail/arm64_decode.hpp"
#include "API/detail/common.hpp"
#include "API/detail/elf_view.hpp"
#include "API/detail/proc_maps.hpp"
#include "API/detail/safe_read.hpp"

#include "xdl.h"

namespace il2cpp_api {
namespace detail {

constexpr int k_collect_cstr_max = 160;
constexpr int k_collect_window_insns = 48;
constexpr int k_collect_expect_bl = 248;
constexpr int k_collect_max_bl_delta = 40;
constexpr unsigned k_collect_fc_band_lo = 0x3330000u;
constexpr unsigned k_collect_fc_band_hi = 0x33c0000u;

/** Same masks as legacy IL2CPP_API.hpp `is_cf_insn` (differs subtly from arm64_decode::is_control_flow on B.cond). */
inline bool collect_is_cf_insn_backup(uint32_t insn) {
    const uint32_t op6 = insn >> 26;
    if (op6 == 0x05u || op6 == 0x25u) return true;
    if ((insn & 0xFF000010u) == 0x54000000u) return true;
    if ((insn & 0x7E000000u) == 0x34000000u) return true;
    if ((insn & 0x7E000000u) == 0x36000000u) return true;
    if ((insn & 0xFE1FFC1Fu) == 0xD61F0000u) return true;
    if ((insn & 0xFE1FFC1Fu) == 0xD63F0000u) return true;
    if ((insn & 0xFFFFFC1Fu) == 0xD65F0000u) return true;
    return false;
}

/** Half-open [scan_lo, scan_hi) inside `seg`; used to narrow BL scan when dynsym/text bounds OK. */
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
    return *scan_lo + UINT64_C(4) <= *scan_hi;
}

inline uintptr_t collect_pick_fn_va_rx(uint64_t candidates[32], int ncan, uint64_t reg_tgt_va,
                                       const MappedRegion *segs, int nseg) {
    uint64_t best_rx = 0;
    uint64_t best_any = 0;
    for (int i = 0; i < ncan; ++i) {
        const uint64_t a = candidates[i];
        if (a == reg_tgt_va) continue;
        if ((a & UINT64_C(3)) != 0) continue;
        if (a > best_any) best_any = a;
        const uintptr_t ap = static_cast<uintptr_t>(a);
        int rx = 0;
        for (int j = 0; j < nseg; ++j) {
            if (segs[j].is_exec && ap >= segs[j].va_start && ap < segs[j].va_end) {
                rx = 1;
                break;
            }
        }
        if (rx && a > best_rx) best_rx = a;
    }
    return best_rx ? static_cast<uintptr_t>(best_rx) : static_cast<uintptr_t>(best_any);
}

inline uintptr_t collect_pick_registration_thunk_from_bl_counts(
    const std::unordered_map<uint64_t, int> &bl_counts, bool elf_ok, uintptr_t load_base,
    const ElfLoadSlice *loads, int nloads, const MappedRegion *segs, int nseg) {
    int best_rank = INT_MIN;
    uintptr_t best_va = 0;
    constexpr int expect = k_collect_expect_bl;
    constexpr int maxd = k_collect_max_bl_delta;

    for (const auto &kv : bl_counts) {
        const uintptr_t tgt = static_cast<uintptr_t>(kv.first);
        const int cnt = kv.second;
        const int d = cnt > expect ? cnt - expect : expect - cnt;
        if (d > maxd || cnt < 60) continue;

        uintptr_t fo = 0;
        const bool okfo =
            va_to_hdr_foff(elf_ok, tgt, load_base, loads, nloads, segs, nseg, &fo);
        const unsigned fou = okfo ? static_cast<unsigned>(fo) : 0u;
        const bool inband = okfo && fou >= k_collect_fc_band_lo && fou <= k_collect_fc_band_hi;
        const int rank = static_cast<int>(inband ? 100000 : 0) - d * 100 + cnt;
        if (rank > best_rank) {
            best_rank = rank;
            best_va = tgt;
        }
    }

    if (best_va != 0) return best_va;

    int max_cnt = -1;
    uintptr_t max_va = 0;
    for (const auto &kv : bl_counts) {
        if (kv.second > max_cnt) {
            max_cnt = kv.second;
            max_va = static_cast<uintptr_t>(kv.first);
        }
    }
    return max_va;
}

/**
 * Raw deref IL2CPP API string inside libil2cpp VMA envelope (covers mmap gaps omitted from slice list).
 */
inline bool collect_read_cstr_il2cpp_va_raw(uintptr_t runtime_va, uintptr_t vma_lo, uintptr_t vma_hi,
                                            bool vma_bounds_ok, char *out, size_t olen,
                                            size_t *out_len) {
    if (!out || olen < UINT64_C(2)) return false;
    if (out_len) *out_len = 0;
    if (!vma_bounds_ok || runtime_va < vma_lo || runtime_va >= vma_hi) return false;

    const size_t max_bounded =
        std::min(static_cast<size_t>(k_collect_cstr_max), olen);
    const size_t len = safe_read_cstr_bounded(runtime_va, out, max_bounded);
    if (len == UINT64_C(0)) return false;

    char nz = '\1';
    if (!safe_read_va(runtime_va + len, &nz, UINT64_C(1)) || nz != '\0') return false;

    for (size_t i = UINT64_C(0); i < len; ++i) {
        const unsigned char c = static_cast<unsigned char>(out[i]);
        if (c < UINT8_C(0x20) || c > UINT8_C(0x7e)) return false;
    }
    out[len] = '\0';

    if (!is_valid_api_name(out)) return false;
    if (out_len) *out_len = len;
    return true;
}

inline bool collect_read_preview16_vma_raw_fallback(uintptr_t addr, uintptr_t vma_lo,
                                                    uintptr_t vma_hi, bool vma_bounds_ok,
                                                    const MappedRegion *segs, int nseg,
                                                    uint8_t *out16) {
    if (!out16 || !vma_bounds_ok || addr < vma_lo || addr >= vma_hi) return false;

    if (memcpy_from_regions(segs, nseg, addr, out16, UINT64_C(16))) return true;

    return safe_read_va(addr, out16, UINT64_C(16));
}

inline void aarch64_registration_pass2(
    uintptr_t reg_tgt_va, const MappedRegion *segs, int nseg, uintptr_t vma_lo, uintptr_t vma_hi,
    bool vma_bounds_ok, bool elf_ok, uintptr_t load_base, const ElfLoadSlice *loads, int nloads,
    bool text_bounds_ok, uintptr_t anchor_tva, uintptr_t anchor_tve,
    std::unordered_map<std::string, uintptr_t> &seen, ApiList &raw_out,
    uintptr_t vma_lo_floor_lookback) {

    const size_t wbuf_sz = static_cast<size_t>(k_collect_window_insns) * UINT64_C(4) + UINT64_C(64);
    std::vector<unsigned char> wbuf(wbuf_sz);

    size_t n_bl_thunk = 0;
    for (int si = 0; si < nseg; ++si) {
        const MappedRegion &seg = segs[si];
        uintptr_t wlo = 0;
        uintptr_t whi = 0;
        if (!scanner_region_text_window(seg, text_bounds_ok, anchor_tva, anchor_tve, &wlo, &whi))
            continue;
        for (uintptr_t pc_scan = wlo; pc_scan + UINT64_C(4) <= whi; pc_scan += UINT64_C(4)) {
            uint32_t insn_try = 0;
            if (!memcpy_from_regions(segs, nseg, pc_scan, &insn_try, sizeof(insn_try))) continue;
            uint64_t tt = 0;
            if (!decode_bl(static_cast<uint64_t>(pc_scan), insn_try, &tt)) continue;
            if (static_cast<uintptr_t>(tt) != reg_tgt_va) continue;
            ++n_bl_thunk;
        }
    }

    size_t site_idx = 0;
    for (int si = 0; si < nseg; ++si) {
        const MappedRegion &seg = segs[si];
        uintptr_t text_lo_seg = seg.va_start;

        uintptr_t wlo = 0;
        uintptr_t whi = 0;
        if (!scanner_region_text_window(seg, text_bounds_ok, anchor_tva, anchor_tve, &wlo, &whi))
            continue;

        for (uintptr_t bl_pc = wlo; bl_pc + UINT64_C(4) <= whi; bl_pc += UINT64_C(4)) {
            uint32_t insn_bl = 0;
            if (!memcpy_from_regions(segs, nseg, bl_pc, &insn_bl, sizeof(insn_bl))) continue;
            uint64_t tgt = 0;
            if (!decode_bl(static_cast<uint64_t>(bl_pc), insn_bl, &tgt)) continue;
            if (static_cast<uintptr_t>(tgt) != reg_tgt_va) continue;

            const bool diag_site0 = (site_idx == 0);
            ++site_idx;

            if (diag_site0 && bl_pc >= UINT64_C(48)) {
                unsigned char pre48[48];
                if (memcpy_from_regions(segs, nseg, bl_pc - UINT64_C(48), pre48, sizeof(pre48))) {
                    SDKLOGI("[scanner-bytes] site=0 bl_pc=0x%llx", JLOG_HEX(bl_pc));
                    for (unsigned i = 0; i < 12u; ++i) {
                        const uintptr_t insn_pc =
                            bl_pc - (UINT64_C(12) - static_cast<uint64_t>(i)) * UINT64_C(4);
                        const uint32_t insn =
                            static_cast<uint32_t>(pre48[i * 4u]) |
                            (static_cast<uint32_t>(pre48[i * 4u + 1u]) << 8) |
                            (static_cast<uint32_t>(pre48[i * 4u + 2u]) << 16) |
                            (static_cast<uint32_t>(pre48[i * 4u + 3u]) << 24);
                        SDKLOGI("[scanner-bytes] +%02u ipc=0x%llx insn=0x%08x", i,
                             JLOG_HEX(insn_pc), insn);
                    }
                }
            }

            const uintptr_t narrow_floor_va = text_bounds_ok ? wlo : text_lo_seg;

            size_t offs_bl =
                static_cast<size_t>((bl_pc >= text_lo_seg) ? static_cast<size_t>(bl_pc - text_lo_seg)
                                                             : SIZE_MAX);
            const size_t text_sz_seg = seg.len;
            if (offs_bl == SIZE_MAX || offs_bl >= text_sz_seg || offs_bl % UINT64_C(4) != UINT64_C(0))
                continue;

            const size_t max_back_offs =
                static_cast<size_t>(k_collect_window_insns) * UINT64_C(4);
            size_t limit_offs_legacy =
                offs_bl > max_back_offs ? offs_bl - max_back_offs : UINT64_C(0);

            size_t vma_rel_floor = UINT64_C(0);
            if (vma_bounds_ok && vma_lo_floor_lookback > text_lo_seg) {
                vma_rel_floor =
                    static_cast<size_t>(vma_lo_floor_lookback - text_lo_seg);
            }
            size_t narrow_rel_floor = UINT64_C(0);
            if (narrow_floor_va > text_lo_seg) {
                narrow_rel_floor = static_cast<size_t>(narrow_floor_va - text_lo_seg);
            }
            size_t limit_offs = limit_offs_legacy;
            if (vma_rel_floor > limit_offs) limit_offs = vma_rel_floor;
            if (narrow_rel_floor > limit_offs) limit_offs = narrow_rel_floor;

            size_t bounded_start_offs = offs_bl;
            size_t scan_offs = offs_bl >= UINT64_C(4) ? offs_bl - UINT64_C(4) : UINT64_C(0);
            int back_steps = 0;
            while (scan_offs >= limit_offs && scan_offs + UINT64_C(4) <= text_sz_seg) {
                const uintptr_t spc = text_lo_seg + scan_offs;
                uint32_t cf_insn = 0;
                if (!memcpy_from_regions(segs, nseg, spc, &cf_insn, sizeof(cf_insn))) break;
                if (collect_is_cf_insn_backup(cf_insn)) {
                    bounded_start_offs = scan_offs + UINT64_C(4);
                    break;
                }
                if (scan_offs == 0) {
                    bounded_start_offs = 0;
                    break;
                }
                scan_offs -= UINT64_C(4);
                ++back_steps;
                if (back_steps >= k_collect_window_insns) break;
            }
            if (bounded_start_offs == offs_bl) bounded_start_offs = limit_offs;

            const uintptr_t bounded_start_va_final = text_lo_seg + bounded_start_offs;

            const size_t winbytes = static_cast<size_t>(bl_pc - bounded_start_va_final);
            if (winbytes == 0 || winbytes > wbuf_sz) continue;
            if (!memcpy_from_regions(segs, nseg, bounded_start_va_final, wbuf.data(), winbytes))
                continue;

            unsigned char valid[32] = {};
            uint64_t regv[32] = {};
            const size_t nins = winbytes / UINT64_C(4);
            for (size_t j = 0; j < nins; ++j) {
                const uintptr_t ipc = bounded_start_va_final + j * UINT64_C(4);
                unsigned char *wp = wbuf.data() + j * UINT64_C(4);
                uint32_t val = static_cast<uint32_t>(static_cast<unsigned>(wp[0])) |
                               (static_cast<uint32_t>(static_cast<unsigned>(wp[1])) << 8) |
                               (static_cast<uint32_t>(static_cast<unsigned>(wp[2])) << 16) |
                               (static_cast<uint32_t>(static_cast<unsigned>(wp[3])) << 24);
                unsigned rd = 0;
                unsigned rn = 0;
                uint64_t page = 0;
                uint64_t imm = 0;
                uint64_t adaddr = 0;
                if (decode_adrp(static_cast<uint64_t>(ipc), val, &rd, &page)) {
                    if (rd < 32) {
                        regv[rd] = page;
                        valid[rd] = 1;
                    }
                    continue;
                }
                if (decode_add_imm(val, &rd, &rn, &imm)) {
                    if (rd < 32 && rn < 32 && rd == rn && valid[rn]) {
                        regv[rd] = regv[rn] + imm;
                        valid[rd] = 1;
                    }
                    continue;
                }
                if (decode_adr(static_cast<uint64_t>(ipc), val, &rd, &adaddr)) {
                    if (rd < 32) {
                        regv[rd] = adaddr;
                        valid[rd] = 1;
                    }
                }
            }

            if (diag_site0) {
                SDKLOGI("[scanner-bytes] site=0 after forward_track: regv summary");
                for (unsigned r = 0; r < 32u; ++r) {
                    if (!valid[r]) continue;
                    SDKLOGI("[scanner-bytes]   r%u = 0x%llx", r, JLOG_HEX(regv[r]));
                    if (!vma_bounds_ok) continue;
                    const uintptr_t addr = static_cast<uintptr_t>(regv[r]);
                    if (addr < vma_lo || addr >= vma_hi) continue;
                    uint8_t dbg16[16] = {};
                    if (!collect_read_preview16_vma_raw_fallback(addr, vma_lo, vma_hi, vma_bounds_ok,
                                                                   segs, nseg, dbg16))
                        continue;
                    SDKLOGI("[scanner-bytes]   r%u@0x%llx hex="
                          "%02x %02x %02x %02x %02x %02x %02x %02x "
                          "%02x %02x %02x %02x %02x %02x %02x %02x",
                         r, JLOG_HEX(static_cast<uint64_t>(addr)), static_cast<unsigned>(dbg16[0]),
                         static_cast<unsigned>(dbg16[1]), static_cast<unsigned>(dbg16[2]),
                         static_cast<unsigned>(dbg16[3]), static_cast<unsigned>(dbg16[4]),
                         static_cast<unsigned>(dbg16[5]), static_cast<unsigned>(dbg16[6]),
                         static_cast<unsigned>(dbg16[7]), static_cast<unsigned>(dbg16[8]),
                         static_cast<unsigned>(dbg16[9]), static_cast<unsigned>(dbg16[10]),
                         static_cast<unsigned>(dbg16[11]), static_cast<unsigned>(dbg16[12]),
                         static_cast<unsigned>(dbg16[13]), static_cast<unsigned>(dbg16[14]),
                         static_cast<unsigned>(dbg16[15]));
                    char ascii[17] = {};
                    for (int i = 0; i < 16; ++i) {
                        const unsigned char b = dbg16[static_cast<size_t>(i)];
                        ascii[i] =
                            (b >= UINT8_C(0x20) && b < UINT8_C(0x7f))
                                ? static_cast<char>(b)
                                : '.';
                    }
                    SDKLOGI("[scanner-bytes]   r%u@0x%llx ascii=\"%s\"", r,
                         JLOG_HEX(static_cast<uint64_t>(addr)), ascii);
                }
            }

            char names[k_collect_cstr_max];
            names[0] = '\0';
            uint64_t fpool[32]{};
            int nf = 0;

            for (unsigned r = 0; r < 32u; ++r) {
                if (!valid[r]) continue;
                const uintptr_t addr = static_cast<uintptr_t>(regv[r]);
                char strbuf[k_collect_cstr_max];
                if (collect_read_cstr_il2cpp_va_raw(addr, vma_lo, vma_hi, vma_bounds_ok, strbuf,
                                                    sizeof(strbuf), nullptr)) {
                    if (!names[0] || std::strlen(strbuf) > std::strlen(names)) {
                        std::strncpy(names, strbuf, sizeof(names) - UINT64_C(1));
                        names[sizeof(names) - UINT64_C(1)] = '\0';
                    }
                    continue;
                }
                int vma_hit = 0;
                for (int j = 0; j < nseg; ++j) {
                    if (addr >= segs[j].va_start && addr < segs[j].va_end) {
                        vma_hit = 1;
                        break;
                    }
                }
                if (vma_hit != 0 && (addr & UINT64_C(3)) == UINT64_C(0) &&
                    addr != static_cast<uintptr_t>(reg_tgt_va)) {
                    if (nf < 32) fpool[nf++] = static_cast<uint64_t>(addr);
                }
            }

            const uintptr_t fn_va_u = collect_pick_fn_va_rx(fpool, nf,
                                                            static_cast<uint64_t>(reg_tgt_va), segs,
                                                            nseg);

            uintptr_t fn_fo_chk = 0;
            if (!fn_va_u ||
                !va_to_hdr_foff(elf_ok, fn_va_u, load_base, elf_ok ? loads : nullptr,
                                elf_ok ? nloads : 0, segs, nseg, &fn_fo_chk) || !names[0]) {
                continue;
            }

            dedup_push(raw_out, seen, std::string(names), fn_va_u);
        }
    }

    SDKLOGI("[scanner] pass2 bl_to_thunk_sites=%zu pairs=%zu", n_bl_thunk, raw_out.size());
    const size_t n_show = std::min<size_t>(3, raw_out.size());
    for (size_t pi = 0; pi < n_show; ++pi) {
        SDKLOGI("[scanner] pass2 sample pair[%zu] %s -> 0x%llx", pi, raw_out.at(pi).first.c_str(),
             JLOG_HEX(raw_out.at(pi).second));
    }
}

inline ApiList resolve_stripped_aarch64_manual(void *il2cpp_handle,
                                               uintptr_t il2cpp_base_param) {
    ApiList raw;
    std::unordered_map<std::string, uintptr_t> seen;
    (void)il2cpp_handle;

    uintptr_t il2cpp_base_log = il2cpp_base_param;
        if (il2cpp_base_log == 0 && il2cpp_handle) {
            xdl_info_t info{};
            if (xdl_info(il2cpp_handle, XDL_DI_DLINFO, &info) == 0 && info.dli_fbase) {
                il2cpp_base_log = reinterpret_cast<uintptr_t>(info.dli_fbase);
            }
        }
    SDKLOGI("[scanner] start base=0x%llx (aarch64 legacy collect path)",
         JLOG_HEX(il2cpp_base_log));

    std::vector<MappedRegion> regions;
    if (!read_libil2cpp_regions(regions) || regions.empty()) {
        SDKLOGE("[scanner] no libil2cpp.so regions in maps");
        return raw;
    }

    const MappedRegion *segs = regions.data();
    const int nseg = static_cast<int>(regions.size());

    SDKLOGI("[scanner] regions count=%zu", regions.size());

    uintptr_t vma_lo = 0;
    uintptr_t vma_hi = 0;
    const bool vma_bounds_ok =
        vma_span_lo_hi_vec(regions, &vma_lo, &vma_hi);
    if (!vma_bounds_ok) {
        SDKLOGW("[scanner] vma_span_lo_hi_vec failed; site=0 VMA hex dumps disabled");
    }

    uintptr_t load_base = 0;
    ElfLoadSlice loads[kMaxPtLoadSlices]{};
    int nloads = 0;
    const int bias_rc = compute_load_bias(segs, nseg, &load_base, loads, &nloads);
    const bool elf_ok = (bias_rc == 0);
    if (elf_ok) {
        SDKLOGI("[scanner] compute_load_bias ok load_base=0x%llx n_pt_load=%d", JLOG_HEX(load_base),
             nloads);
    } else {
        SDKLOGW("[scanner] compute_load_bias failed rc=%d; using maps fallback for hdr_foff only",
             bias_rc);
    }

    size_t readable_bytes_total = 0;
    for (const MappedRegion &r : regions) {
        readable_bytes_total += r.len;
    }

    DynsymCandidate dyn_cand{};
    const bool dynsym_anchor_ok =
        elf_ok && find_registration_func_from_dynsym(segs, nseg, load_base, &dyn_cand);

    uintptr_t tva = 0;
    uintptr_t tve = 0;
    bool narrow_bounds = false;
    std::unordered_map<uint64_t, int> bl_counts;
    bl_counts.reserve(8192);

    if (dynsym_anchor_ok) {
        narrow_bounds = true;
        tva = dyn_cand.va_start;
        tve = dyn_cand.va_end;
        SDKLOGI("[scanner] dynsym anchor: func va=[0x%llx,0x%llx) size=%zu KB thunk=0x%llx count=%d",
             JLOG_HEX(tva), JLOG_HEX(tve), static_cast<size_t>(tve - tva) / UINT64_C(1024),
             JLOG_HEX(dyn_cand.thunk_va), dyn_cand.bl_to_thunk_count);

        uintptr_t thunk_fo_fast = 0;
        const bool fo_ok_fast =
            va_to_hdr_foff(elf_ok, dyn_cand.thunk_va, load_base, elf_ok ? loads : nullptr,
                           elf_ok ? nloads : 0, segs, nseg, &thunk_fo_fast);
        const bool in_band_fast =
            fo_ok_fast &&
            thunk_fo_fast >= static_cast<uintptr_t>(k_collect_fc_band_lo) &&
            thunk_fo_fast <= static_cast<uintptr_t>(k_collect_fc_band_hi);
        SDKLOGI("[scanner] thunk va=0x%llx file_off=0x%llx bl_count=%d in_api_band=%d fo_ok=%d "
              "(dynsym anchor fast path)",
             JLOG_HEX(dyn_cand.thunk_va), JLOG_HEX(thunk_fo_fast), dyn_cand.bl_to_thunk_count,
              in_band_fast ? 1 : 0, fo_ok_fast ? 1 : 0);

        if (dyn_cand.thunk_va == 0) {
            SDKLOGW("[scanner] pass1 registration thunk_va=0 (dynsym anchor invalid)");
            return raw;
        }

        aarch64_registration_pass2(dyn_cand.thunk_va, segs, nseg, vma_lo, vma_hi, vma_bounds_ok,
                                   elf_ok, load_base, loads, nloads, true, tva, tve, seen, raw,
                                   vma_lo);

        if (!raw.empty()) {
            std::sort(raw.begin(), raw.end(),
                      [](const ApiEntry &a, const ApiEntry &b) { return a.first < b.first; });
            SDKLOGI("[scanner] done merged_pairs=%zu", raw.size());
            const size_t n_show_dy = std::min<size_t>(3, raw.size());
            for (size_t pi = 0; pi < n_show_dy; ++pi) {
                SDKLOGI("[scanner] sample pair[%zu] %s -> 0x%llx", pi, raw[pi].first.c_str(),
                     JLOG_HEX(raw[pi].second));
            }
            SDKLOGI("[scanner] pass1 skipped (dynsym)");
            return raw;
        }

        SDKLOGW("[scanner] dynsym anchor produced 0 pairs, falling back to pass1 byte scan");
        bl_counts.clear();
    } else if (elf_ok) {
        SDKLOGW("[scanner] dynsym anchor not found, falling back to pass1 bounds");
    }

    narrow_bounds = find_text_section_bounds(segs, nseg, &tva, &tve);
    if (narrow_bounds) {
        SDKLOGI("[scanner] text section va=[0x%llx,0x%llx) size=%zu KB", JLOG_HEX(tva), JLOG_HEX(tve),
              static_cast<size_t>(tve - tva) / UINT64_C(1024));
    } else if (elf_ok && find_pt_load_x_bounds(load_base, loads, nloads, &tva, &tve)) {
        narrow_bounds = true;
        SDKLOGI("[scanner] using PT_LOAD PF_X bounds va=[0x%llx,0x%llx) size=%zu KB", JLOG_HEX(tva),
             JLOG_HEX(tve), static_cast<size_t>(tve - tva) / UINT64_C(1024));
    } else {
        SDKLOGW("[scanner] no text/PT_LOAD-X bounds, scanning all regions");
    }

    size_t pass1_in_text_bytes = 0;
    for (int si = 0; si < nseg; ++si) {
        const MappedRegion &seg = segs[si];
        uintptr_t wlo = 0;
        uintptr_t whi = 0;
        if (!scanner_region_text_window(seg, narrow_bounds, tva, tve, &wlo, &whi)) continue;
        pass1_in_text_bytes += static_cast<size_t>(whi - wlo);
        for (uintptr_t pc = wlo; pc + UINT64_C(4) <= whi; pc += UINT64_C(4)) {
            uint32_t insn = 0;
            if (!memcpy_from_regions(segs, nseg, pc, &insn, sizeof(insn))) continue;
            uint64_t tgt = 0;
            if (!decode_bl(static_cast<uint64_t>(pc), insn, &tgt)) continue;
            bl_counts[tgt]++;
        }
    }

    const size_t pass1_skipped_mb =
        readable_bytes_total > pass1_in_text_bytes
            ? (readable_bytes_total - pass1_in_text_bytes) / (UINT64_C(1024) * UINT64_C(1024))
            : UINT64_C(0);
    SDKLOGI("[scanner] pass1 in_text_bytes=%zu (skipped %zu MB)", pass1_in_text_bytes,
         pass1_skipped_mb);

    if (bl_counts.empty()) {
        SDKLOGW("[scanner] pass1 no BL branch targets (empty histogram)");
        return raw;
    }

    int cnt = 0;
    const uintptr_t reg_tgt_va =
        collect_pick_registration_thunk_from_bl_counts(bl_counts, elf_ok, load_base,
                                                       elf_ok ? loads : nullptr, elf_ok ? nloads : 0,
                                                       segs, nseg);

    for (const auto &kv : bl_counts) {
        if (kv.first == static_cast<uint64_t>(reg_tgt_va)) {
            cnt = kv.second;
            break;
        }
    }

    if (cnt == 0 || reg_tgt_va == 0) {
        SDKLOGW("[scanner] pass1 thunk pick failed cnt=%d va=0x%llx bl_unique=%zu", cnt,
             JLOG_HEX(reg_tgt_va), bl_counts.size());
        return raw;
    }

    uintptr_t reg_fo = 0;
    if (!va_to_hdr_foff(elf_ok, reg_tgt_va, load_base, elf_ok ? loads : nullptr,
                        elf_ok ? nloads : 0, segs, nseg, &reg_fo)) {
        reg_fo = 0;
    }
    SDKLOGI("[scanner] register thunk fo=0x%llx VA=0x%llx BL_sites=%d", JLOG_HEX(reg_fo),
         JLOG_HEX(reg_tgt_va), cnt);

    aarch64_registration_pass2(reg_tgt_va, segs, nseg, vma_lo, vma_hi, vma_bounds_ok, elf_ok,
                               load_base, loads, nloads, narrow_bounds, tva, tve, seen, raw,
                               vma_lo);

    std::sort(raw.begin(), raw.end(),
              [](const ApiEntry &a, const ApiEntry &b) { return a.first < b.first; });

    SDKLOGI("[scanner] done merged_pairs=%zu", raw.size());
    const size_t n_show = std::min<size_t>(3, raw.size());
    for (size_t pi = 0; pi < n_show; ++pi) {
        SDKLOGI("[scanner] sample pair[%zu] %s -> 0x%llx", pi, raw[pi].first.c_str(),
             JLOG_HEX(raw[pi].second));
    }
    return raw;
}

}  // namespace detail
}  // namespace il2cpp_api

#endif  // __aarch64__
