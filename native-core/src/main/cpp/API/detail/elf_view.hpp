// ELF PT_LOAD slicing + runtime load bias anchoring against `/proc`-mapped chunks.
#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <unordered_map>
#include <vector>

#include <elf.h>

#include "Logger.h"
#include "API/detail/proc_maps.hpp"

#if defined(__arm__)

#include "API/detail/arm32_decode.hpp"

#elif defined(__aarch64__)

#include "API/detail/arm64_decode.hpp"

#endif

#if defined(__arm__) || defined(__aarch64__)

#include "API/detail/dynsym_resolve.hpp"

#endif

namespace il2cpp_api {
namespace detail {

constexpr int kMaxPtLoadSlices = 32;

struct ElfLoadSlice {
    uint64_t p_vaddr = 0;
    uint64_t p_offset = 0;
    uint64_t p_filesz = 0;
    uint64_t p_memsz = 0;
    uint32_t p_flags = 0;
};

inline int compute_load_bias_finish(const MappedRegion *segs, int nseg, uintptr_t *load_base_out,
                                    ElfLoadSlice *loads_out, int *nloads_out) {
    if (!segs || nseg <= 0 || !load_base_out || !loads_out || !nloads_out || *nloads_out <= 0)
        return -1;

    int anch = -1;
    for (int j = 0; j < *nloads_out; ++j) {
        if (loads_out[j].p_filesz == 0) continue;
        if (!(loads_out[j].p_flags & PF_X)) continue;
        if (anch < 0 || loads_out[j].p_offset < loads_out[anch].p_offset) anch = j;
    }
    if (anch < 0) {
        for (int j = 0; j < *nloads_out; ++j) {
            if (loads_out[j].p_filesz == 0) continue;
            if (loads_out[j].p_offset == 0 && loads_out[j].p_vaddr == 0) continue;
            if (anch < 0 || loads_out[j].p_offset < loads_out[anch].p_offset) anch = j;
        }
    }
    if (anch < 0) {
        for (int j = 0; j < *nloads_out; ++j) {
            if (loads_out[j].p_filesz == 0) continue;
            if (anch < 0 || loads_out[j].p_vaddr < loads_out[anch].p_vaddr) anch = j;
        }
    }
    if (anch < 0) return -1;

    const uint64_t po = loads_out[anch].p_offset;
    const uint64_t pv = loads_out[anch].p_vaddr;
    int best_s = -1;
    uintptr_t best_foff_val = 0;
    for (int s = 0; s < nseg; ++s) {
        if (po < segs[s].file_off) continue;
        if (po >= segs[s].file_off + segs[s].len) continue;
        if (best_s < 0 || segs[s].file_off > best_foff_val) {
            best_s = s;
            best_foff_val = segs[s].file_off;
        }
    }
    if (best_s < 0) return -1;

    uintptr_t va_at_po = segs[best_s].va_start + static_cast<uintptr_t>(po - segs[best_s].file_off);

    uintptr_t pv_u = static_cast<uintptr_t>(pv);
    if (pv_u > va_at_po) return -1;

    uintptr_t lb = va_at_po - pv_u;

    *load_base_out = lb;
    return 0;
}

inline int compute_load_bias(const MappedRegion *segs, int nseg, uintptr_t *load_base_out,
                             ElfLoadSlice *loads_out, int *nloads_out) {
    if (!segs || nseg <= 0 || !load_base_out || !loads_out || !nloads_out) return -1;

    uintptr_t img_va = UINTPTR_MAX;
    for (int i = 0; i < nseg; ++i) {
        if (segs[i].file_off != 0) continue;
        if (segs[i].va_start < img_va) img_va = segs[i].va_start;
    }
    if (img_va == UINTPTR_MAX) return -1;

    unsigned char buf[8192]{};
    if (!memcpy_from_regions(segs, nseg, img_va, buf, sizeof(buf))) return -1;
    const unsigned char *b = buf;
    if (std::memcmp(b, ELFMAG, SELFMAG) != 0 || b[EI_DATA] != ELFDATA2LSB ||
        b[EI_VERSION] != EV_CURRENT)
        return -1;

    *nloads_out = 0;

    if (b[EI_CLASS] == ELFCLASS64) {
        const Elf64_Ehdr *eh = reinterpret_cast<const Elf64_Ehdr *>(b);
        if (eh->e_phoff >= sizeof(buf) || eh->e_phnum == 0 ||
            eh->e_phentsize < sizeof(Elf64_Phdr) ||
            eh->e_phoff + static_cast<uint64_t>(eh->e_phnum) * eh->e_phentsize > sizeof(buf))
            return -1;

        const Elf64_Phdr *phdrs =
            reinterpret_cast<const Elf64_Phdr *>(b + eh->e_phoff);

        for (uint16_t i = 0; i < eh->e_phnum && *nloads_out < kMaxPtLoadSlices; ++i) {
            const Elf64_Phdr &ph = phdrs[i];
            if (ph.p_type != PT_LOAD || ph.p_memsz == 0) continue;

            ElfLoadSlice &dst = loads_out[*nloads_out];
            dst.p_vaddr = ph.p_vaddr;
            dst.p_offset = ph.p_offset;
            dst.p_filesz = ph.p_filesz;
            dst.p_memsz = ph.p_memsz;
            dst.p_flags = ph.p_flags;
            (*nloads_out)++;
        }
    } else if (b[EI_CLASS] == ELFCLASS32) {
        const Elf32_Ehdr *eh = reinterpret_cast<const Elf32_Ehdr *>(b);
        if (eh->e_phoff >= sizeof(buf) || eh->e_phnum == 0 ||
            eh->e_phentsize < sizeof(Elf32_Phdr) ||
            eh->e_phoff + static_cast<uint64_t>(eh->e_phnum) * eh->e_phentsize > sizeof(buf))
            return -1;

        const Elf32_Phdr *phdrs =
            reinterpret_cast<const Elf32_Phdr *>(b + eh->e_phoff);

        for (uint16_t i = 0; i < eh->e_phnum && *nloads_out < kMaxPtLoadSlices; ++i) {
            const Elf32_Phdr &ph = phdrs[i];
            if (ph.p_type != PT_LOAD || ph.p_memsz == 0) continue;

            ElfLoadSlice &dst = loads_out[*nloads_out];
            dst.p_vaddr = ph.p_vaddr;
            dst.p_offset = ph.p_offset;
            dst.p_filesz = ph.p_filesz;
            dst.p_memsz = ph.p_memsz;
            dst.p_flags = ph.p_flags;
            (*nloads_out)++;
        }
    } else {
        return -1;
    }

    if (*nloads_out == 0) return -1;
    return compute_load_bias_finish(segs, nseg, load_base_out, loads_out, nloads_out);
}

/** Bounding VA range spanning all PF_X PT_LOAD segments: [va_start, va_end). */
inline bool find_pt_load_x_bounds(uintptr_t load_base, const ElfLoadSlice *loads, int nloads,
                                  uintptr_t *va_start, uintptr_t *va_end) {
    if (!loads || nloads <= 0 || !va_start || !va_end) return false;

    bool any = false;
    uint64_t min_pv = 0;
    uint64_t max_end = 0;

    for (int i = 0; i < nloads; ++i) {
        if (!(loads[i].p_flags & PF_X)) continue;
        const uint64_t v = loads[i].p_vaddr;
        const uint64_t ms = loads[i].p_memsz;
        if (ms == 0) continue;
        const uint64_t end = v + ms;
        if (end < v) continue;

        if (!any) {
            min_pv = v;
            max_end = end;
            any = true;
        } else {
            if (v < min_pv) min_pv = v;
            if (end > max_end) max_end = end;
        }
    }
    if (!any) return false;

    const uint64_t lb = static_cast<uint64_t>(load_base);
    const uint64_t lo64 = lb + min_pv;
    const uint64_t hi64 = lb + max_end;
    if (lo64 < lb || hi64 <= lo64 || hi64 < lb || hi64 > static_cast<uint64_t>(UINTPTR_MAX))
        return false;

    *va_start = static_cast<uintptr_t>(lo64);
    *va_end = static_cast<uintptr_t>(hi64);
    return true;
}

inline bool va_to_rva(uintptr_t va, uintptr_t load_base, const ElfLoadSlice *loads, int nloads,
                      uint64_t *rva_out) {
    if (!loads || nloads <= 0 || !rva_out || va < load_base) return false;

    uint64_t r = static_cast<uint64_t>(va - load_base);
    for (int i = 0; i < nloads; ++i) {
        const uint64_t vlo = loads[i].p_vaddr;
        const uint64_t vhi = loads[i].p_vaddr + loads[i].p_memsz;
        if (r >= vlo && r < vhi) {
            *rva_out = r;
            return true;
        }
    }
    return false;
}

inline bool va_to_file_off_vma(uintptr_t va, uintptr_t load_base,
                               const ElfLoadSlice *loads, int nloads, uintptr_t *out_fo) {
    uint64_t rva = 0;
    if (!va_to_rva(va, load_base, loads, nloads, &rva) || !out_fo) return false;

    for (int i = 0; i < nloads; ++i) {
        uint64_t vlo = loads[i].p_vaddr;
        uint64_t vhi = loads[i].p_vaddr + loads[i].p_memsz;
        if (!(rva >= vlo && rva < vhi)) continue;
        uint64_t delta = rva - loads[i].p_vaddr;
        if (delta >= loads[i].p_filesz) return false;
        uint64_t fo = loads[i].p_offset + delta;
        if (fo > static_cast<uint64_t>(UINT32_MAX)) return false;
        *out_fo = static_cast<uintptr_t>(fo);
        return true;
    }
    return false;
}

inline bool va_to_file_off(uintptr_t va, const MappedRegion *regions, int nregs, uintptr_t *out_fo) {
    if (!regions || !out_fo || nregs <= 0) return false;

    int best = -1;
    uintptr_t best_foff_val = 0;
    for (int i = 0; i < nregs; ++i) {
        if (va < regions[i].va_start || va >= regions[i].va_end) continue;
        if (best < 0 || regions[i].file_off > best_foff_val) {
            best = i;
            best_foff_val = regions[i].file_off;
        }
    }
    if (best < 0) return false;
    uintptr_t fo = regions[best].file_off + (va - regions[best].va_start);
    if (fo > static_cast<uintptr_t>(UINT32_MAX)) return false;
    *out_fo = fo;
    return true;
}

inline bool va_to_hdr_foff(bool elf_ok, uintptr_t va, uintptr_t load_base,
                           const ElfLoadSlice *loads, int nloads, const MappedRegion *regions,
                           int nregs, uintptr_t *out_fo) {
    if (!out_fo) return false;
    if (elf_ok && va_to_file_off_vma(va, load_base, loads, nloads, out_fo)) return true;
    return va_to_file_off(va, regions, nregs, out_fo);
}

inline bool vma_span_lo_hi(const MappedRegion *regions, int nregs, uintptr_t *vma_lo_out,
                           uintptr_t *vma_hi_out) {
    uintptr_t lo = UINTPTR_MAX;
    uintptr_t hi = 0;
    for (int i = 0; i < nregs; ++i) {
        if (regions[i].va_start < lo) lo = regions[i].va_start;
        if (regions[i].va_end > hi) hi = regions[i].va_end;
    }
    if (vma_lo_out) *vma_lo_out = lo;
    if (vma_hi_out) *vma_hi_out = hi;
    return lo != UINTPTR_MAX && hi > lo;
}

inline bool vma_span_lo_hi_vec(const std::vector<MappedRegion> &regs, uintptr_t *vma_lo_out,
                               uintptr_t *vma_hi_out) {
    const MappedRegion *p = regs.empty() ? nullptr : regs.data();
    return vma_span_lo_hi(p, static_cast<int>(regs.size()), vma_lo_out, vma_hi_out);
}

inline bool va_range_overlaps_regions(const MappedRegion *regs, int nregs, uintptr_t lo,
                                        uintptr_t hi) {
    if (!regs || nregs <= 0 || lo >= hi) return false;
    for (int i = 0; i < nregs; ++i) {
        if (regs[i].va_end > lo && regs[i].va_start < hi) return true;
    }
    return false;
}

/** Map a file offset from the ELF to a runtime VA using `/proc` region file_off spans. */
inline bool file_off_to_va_mapped(const MappedRegion *regs, int nregs, uint64_t file_off,
                                  uintptr_t *va_out) {
    if (!regs || nregs <= 0 || !va_out) return false;
    for (int i = 0; i < nregs; ++i) {
        const uint64_t lo = regs[i].file_off;
        const uint64_t hi = regs[i].file_off + regs[i].len;
        if (file_off >= lo && file_off < hi) {
            const uint64_t delta = file_off - lo;
            *va_out = regs[i].va_start + static_cast<uintptr_t>(delta);
            return true;
        }
    }
    return false;
}

/**
 * Locate runtime VA range [text_va_start, text_va_end) for section ".text" via ELF section headers.
 * On failure, leaves *text_va_start / *text_va_end unset and returns false.
 */
inline bool find_text_section_bounds(const MappedRegion *regions, int nregs, uintptr_t *text_va_start,
                                     uintptr_t *text_va_end) {
    if (!regions || nregs <= 0 || !text_va_start || !text_va_end) return false;

    uintptr_t img_va = UINTPTR_MAX;
    for (int i = 0; i < nregs; ++i) {
        if (regions[i].file_off != 0) continue;
        if (regions[i].va_start < img_va) img_va = regions[i].va_start;
    }
    if (img_va == UINTPTR_MAX) return false;

    std::vector<uint8_t> ehdr_dyn(8192);
    const size_t ehdr_read = sizeof(Elf64_Ehdr) > ehdr_dyn.size() ? sizeof(Elf64_Ehdr) : ehdr_dyn.size();
    if (!memcpy_from_regions(regions, nregs, img_va, ehdr_dyn.data(), ehdr_read)) return false;

    const unsigned char *ehb = ehdr_dyn.data();
    if (std::memcmp(ehb, ELFMAG, SELFMAG) != 0 || ehb[EI_DATA] != ELFDATA2LSB ||
        ehb[EI_VERSION] != EV_CURRENT)
        return false;

    ElfLoadSlice loads[kMaxPtLoadSlices]{};
    int nloads = 0;
    uintptr_t load_base = 0;
    const bool have_bias = (compute_load_bias(regions, nregs, &load_base, loads, &nloads) == 0);

    constexpr size_t k_max_sh_table = 64u * 1024u * 1024u;
    constexpr uint64_t k_max_text = 256u * 1024u * 1024u;

    auto find_dot_text_name = [](const char *tab, size_t tabsz, uint32_t name_off) -> bool {
        if (name_off >= tabsz) return false;
        const char *p = tab + name_off;
        const char *end = tab + tabsz;
        const char *q = p;
        while (q < end && *q != '\0') ++q;
        const size_t n = static_cast<size_t>(q - p);
        return n == 5 && std::memcmp(p, ".text", 5) == 0;
    };

    if (ehb[EI_CLASS] == ELFCLASS64) {
        if (ehdr_dyn.size() < sizeof(Elf64_Ehdr)) return false;
        const Elf64_Ehdr *eh = reinterpret_cast<const Elf64_Ehdr *>(ehb);
        const uint64_t e_shoff = eh->e_shoff;
        const uint16_t e_shnum = eh->e_shnum;
        const uint16_t e_shentsize = eh->e_shentsize;
        const uint16_t e_shstrndx = eh->e_shstrndx;

        if (e_shnum == 0 || e_shentsize < sizeof(Elf64_Shdr) || e_shstrndx == SHN_UNDEF ||
            e_shstrndx >= e_shnum || e_shstrndx == SHN_XINDEX)
            return false;

        const uint64_t table_bytes64 = static_cast<uint64_t>(e_shnum) * e_shentsize;
        if (table_bytes64 / e_shnum != static_cast<uint64_t>(e_shentsize) ||
            table_bytes64 > k_max_sh_table)
            return false;
        const size_t table_bytes = static_cast<size_t>(table_bytes64);

        uintptr_t sh_table_va = 0;
        if (!file_off_to_va_mapped(regions, nregs, e_shoff, &sh_table_va)) return false;

        std::vector<uint8_t> shtab(table_bytes);
        if (!memcpy_from_regions(regions, nregs, sh_table_va, shtab.data(), table_bytes))
            return false;

        const Elf64_Shdr *shstr =
            reinterpret_cast<const Elf64_Shdr *>(shtab.data() + static_cast<size_t>(e_shstrndx) * e_shentsize);
        const uint64_t str_fo = shstr->sh_offset;
        const uint64_t str_sz = shstr->sh_size;
        if (str_sz == 0 || str_sz > k_max_sh_table) return false;

        uintptr_t str_va = 0;
        if (!file_off_to_va_mapped(regions, nregs, str_fo, &str_va)) return false;

        std::vector<uint8_t> strbuf(static_cast<size_t>(str_sz));
        if (!memcpy_from_regions(regions, nregs, str_va, strbuf.data(), static_cast<size_t>(str_sz)))
            return false;
        const char *strtab = reinterpret_cast<const char *>(strbuf.data());

        for (uint16_t i = 0; i < e_shnum; ++i) {
            const Elf64_Shdr *sh =
                reinterpret_cast<const Elf64_Shdr *>(shtab.data() + static_cast<size_t>(i) * e_shentsize);
            if (!find_dot_text_name(strtab, static_cast<size_t>(str_sz), sh->sh_name)) continue;

            const uint64_t sh_size = sh->sh_size;
            if (sh_size == 0 || sh_size > k_max_text) return false;

            uintptr_t tva = 0;
            if (have_bias && (sh->sh_flags & SHF_ALLOC) != 0) {
                const uint64_t addr64 = sh->sh_addr;
                if (addr64 <= static_cast<uint64_t>(UINTPTR_MAX)) {
                    tva = load_base + static_cast<uintptr_t>(addr64);
                }
            }
            if (tva == 0) {
                if (!file_off_to_va_mapped(regions, nregs, sh->sh_offset, &tva)) return false;
            }

            const uint64_t end64 = static_cast<uint64_t>(tva) + sh_size;
            if (end64 <= static_cast<uint64_t>(tva) || end64 > static_cast<uint64_t>(UINTPTR_MAX))
                return false;

            if (!va_range_overlaps_regions(regions, nregs, tva, static_cast<uintptr_t>(end64)))
                return false;

            *text_va_start = tva;
            *text_va_end = static_cast<uintptr_t>(end64);
            return true;
        }
        return false;
    }

    if (ehb[EI_CLASS] == ELFCLASS32) {
        if (ehdr_dyn.size() < sizeof(Elf32_Ehdr)) return false;
        const Elf32_Ehdr *eh = reinterpret_cast<const Elf32_Ehdr *>(ehb);
        const Elf32_Off e_shoff = eh->e_shoff;
        const uint16_t e_shnum = eh->e_shnum;
        const uint16_t e_shentsize = eh->e_shentsize;
        const uint16_t e_shstrndx = eh->e_shstrndx;

        if (e_shnum == 0 || e_shentsize < sizeof(Elf32_Shdr) || e_shstrndx == SHN_UNDEF ||
            e_shstrndx >= e_shnum || e_shstrndx == SHN_XINDEX)
            return false;

        const uint64_t table_bytes64 = static_cast<uint64_t>(e_shnum) * e_shentsize;
        if (table_bytes64 / e_shnum != static_cast<uint64_t>(e_shentsize) ||
            table_bytes64 > k_max_sh_table)
            return false;
        const size_t table_bytes = static_cast<size_t>(table_bytes64);

        uintptr_t sh_table_va = 0;
        if (!file_off_to_va_mapped(regions, nregs, e_shoff, &sh_table_va)) return false;

        std::vector<uint8_t> shtab(table_bytes);
        if (!memcpy_from_regions(regions, nregs, sh_table_va, shtab.data(), table_bytes))
            return false;

        const Elf32_Shdr *shstr =
            reinterpret_cast<const Elf32_Shdr *>(shtab.data() + static_cast<size_t>(e_shstrndx) * e_shentsize);
        const uint64_t str_fo = shstr->sh_offset;
        const uint64_t str_sz = shstr->sh_size;
        if (str_sz == 0 || str_sz > k_max_sh_table) return false;

        uintptr_t str_va = 0;
        if (!file_off_to_va_mapped(regions, nregs, str_fo, &str_va)) return false;

        std::vector<uint8_t> strbuf(static_cast<size_t>(str_sz));
        if (!memcpy_from_regions(regions, nregs, str_va, strbuf.data(), static_cast<size_t>(str_sz)))
            return false;
        const char *strtab = reinterpret_cast<const char *>(strbuf.data());

        for (uint16_t i = 0; i < e_shnum; ++i) {
            const Elf32_Shdr *sh =
                reinterpret_cast<const Elf32_Shdr *>(shtab.data() + static_cast<size_t>(i) * e_shentsize);
            if (!find_dot_text_name(strtab, static_cast<size_t>(str_sz), sh->sh_name)) continue;

            const uint64_t sh_size = sh->sh_size;
            if (sh_size == 0 || sh_size > k_max_text) return false;

            uintptr_t tva = 0;
            if (have_bias && (sh->sh_flags & SHF_ALLOC) != 0) {
                tva = load_base + static_cast<uintptr_t>(sh->sh_addr);
            }
            if (tva == 0) {
                if (!file_off_to_va_mapped(regions, nregs, sh->sh_offset, &tva)) return false;
            }

            const uint64_t end64 = static_cast<uint64_t>(tva) + sh_size;
            if (end64 <= static_cast<uint64_t>(tva) || end64 > static_cast<uint64_t>(UINTPTR_MAX))
                return false;

            if (!va_range_overlaps_regions(regions, nregs, tva, static_cast<uintptr_t>(end64)))
                return false;

            *text_va_start = tva;
            *text_va_end = static_cast<uintptr_t>(end64);
            return true;
        }
        return false;
    }

    return false;
}

struct DynsymCandidate {
    uintptr_t va_start = 0;
    uintptr_t va_end = 0;
    uintptr_t thunk_va = 0;
    int bl_to_thunk_count = 0;
};

#if defined(__arm__) || defined(__aarch64__)

constexpr size_t kRegDynsymMinFuncSize = 4096;
constexpr unsigned kRegDynsymMaxFuncsToHistogram = 50;
constexpr int kRegDynsymBlPeakThreshold = 200;
constexpr uint64_t kRegDynsymMaxSymbolSizeScan = 4u * 1024 * 1024;

inline bool reg_scan_bl_decode_tgt(uintptr_t addr, uint32_t insn, uint64_t *tgt) {
    if (!tgt) return false;
#if defined(__arm__)
    return decode_bl_arm(static_cast<uint32_t>(addr), insn, tgt);
#elif defined(__aarch64__)
    return decode_bl(static_cast<uint64_t>(addr), insn, tgt);
#else
    (void)addr;
    (void)insn;
    (void)tgt;
    return false;
#endif
}

template <typename SymT>
inline void collect_dynsym_big_funcs_elf(uintptr_t load_base, const DynSymInfoDynsym &dyn,
                                         const ModuleRangeDynsym &range,
                                         uint32_t nsyms, const MappedRegion *regions,
                                         int nregs,
                                         std::vector<std::pair<uintptr_t, size_t>> *out_big) {
    if (!dyn.symtab || !out_big) return;
    size_t max_possible =
        dyn.syment > 0 ? static_cast<size_t>((range.hi - dyn.symtab) / dyn.syment) : 0;
    if (max_possible == 0) return;
    if (nsyms > max_possible) nsyms = static_cast<uint32_t>(max_possible);

    for (uint32_t i = 0; i < nsyms; ++i) {
        const uintptr_t sym_addr = dyn.symtab + static_cast<uintptr_t>(i) * dyn.syment;
        if (sym_addr + sizeof(SymT) >= range.hi) break;
        const SymT *sym = reinterpret_cast<const SymT *>(sym_addr);

        unsigned st_type = 0;
        if constexpr (sizeof(SymT) == sizeof(Elf64_Sym)) {
            st_type = ELF64_ST_TYPE(sym->st_info);
        } else {
            st_type = ELF32_ST_TYPE(sym->st_info);
        }
        if (st_type != STT_FUNC) continue;

        const uint64_t st_sz64 = sym->st_size;
        if (st_sz64 < static_cast<uint64_t>(kRegDynsymMinFuncSize)) continue;

        uintptr_t ptr = resolve_sym_value_dynsym<SymT>(
            load_base, static_cast<uintptr_t>(sym->st_value), range);
        if (ptr == 0) continue;

        size_t bodysz = static_cast<size_t>(st_sz64);
        if (st_sz64 != static_cast<uint64_t>(bodysz) || bodysz < kRegDynsymMinFuncSize) continue;
        bodysz = std::min(bodysz, static_cast<size_t>(kRegDynsymMaxSymbolSizeScan));
        const uintptr_t f_hi = ptr + bodysz;
        if (f_hi < ptr || !va_range_overlaps_regions(regions, nregs, ptr, f_hi)) continue;

        out_big->push_back(std::make_pair(ptr, bodysz));
    }
}

/**
 * Locate registration-like dynsym function by BL peeling (≥200 identical targets) inside large
 * STT_FUNC bodies. Parses dynamic table like dynsym_resolve.hpp (ELF32/ELF64).
 */
inline bool find_registration_func_from_dynsym(const MappedRegion *regions, int nregs,
                                             uintptr_t load_base, DynsymCandidate *out) {
    if (!regions || nregs <= 0 || !out) return false;

    uintptr_t img_va = UINTPTR_MAX;
    for (int i = 0; i < nregs; ++i) {
        if (regions[i].file_off != 0) continue;
        if (regions[i].va_start < img_va) img_va = regions[i].va_start;
    }
    if (img_va == UINTPTR_MAX) return false;

    unsigned char buf[8192]{};
    if (!memcpy_from_regions(regions, nregs, img_va, buf, sizeof(buf))) return false;
    if (std::memcmp(buf, ELFMAG, SELFMAG) != 0 || buf[EI_DATA] != ELFDATA2LSB ||
        buf[EI_VERSION] != EV_CURRENT)
        return false;

    std::vector<std::pair<uintptr_t, size_t>> big_funcs;
    big_funcs.reserve(128);

    if (buf[EI_CLASS] == ELFCLASS64) {
        const Elf64_Ehdr *eh = reinterpret_cast<const Elf64_Ehdr *>(buf);
        if (eh->e_phoff >= sizeof(buf) || eh->e_phnum == 0 ||
            eh->e_phentsize < sizeof(Elf64_Phdr) ||
            eh->e_phoff + static_cast<uint64_t>(eh->e_phnum) * eh->e_phentsize > sizeof(buf))
            return false;
        const Elf64_Phdr *phdrs = reinterpret_cast<const Elf64_Phdr *>(buf + eh->e_phoff);
        ModuleRangeDynsym range =
            calc_module_range_dynsym(load_base, phdrs, static_cast<size_t>(eh->e_phnum));
        if (range.lo == 0 || range.hi == 0 || range.lo >= range.hi) return false;

        DynSymInfoDynsym dyn{};
        if (!read_dynamic_info_dynsym<Elf64_Phdr, Elf64_Dyn>(load_base, phdrs,
                                                             static_cast<size_t>(eh->e_phnum),
                                                             range, &dyn, nullptr, nullptr))
            return false;

        uint32_t nsyms = sym_count_from_hash_dynsym(dyn.hash, range);
        if (nsyms == 0)
            nsyms =
                sym_count_from_gnu_hash_dynsym<uint64_t>(dyn.gnu_hash, range);
        if (nsyms == 0) return false;

        collect_dynsym_big_funcs_elf<Elf64_Sym>(load_base, dyn, range, nsyms, regions, nregs,
                                                &big_funcs);
    } else if (buf[EI_CLASS] == ELFCLASS32) {
        const Elf32_Ehdr *eh = reinterpret_cast<const Elf32_Ehdr *>(buf);
        if (eh->e_phoff >= sizeof(buf) || eh->e_phnum == 0 ||
            eh->e_phentsize < sizeof(Elf32_Phdr) ||
            eh->e_phoff + static_cast<uint64_t>(eh->e_phnum) * eh->e_phentsize > sizeof(buf))
            return false;
        const Elf32_Phdr *phdrs = reinterpret_cast<const Elf32_Phdr *>(buf + eh->e_phoff);
        ModuleRangeDynsym range =
            calc_module_range_dynsym(load_base, phdrs, static_cast<size_t>(eh->e_phnum));
        if (range.lo == 0 || range.hi == 0 || range.lo >= range.hi) return false;

        DynSymInfoDynsym dyn{};
        if (!read_dynamic_info_dynsym<Elf32_Phdr, Elf32_Dyn>(load_base, phdrs,
                                                             static_cast<size_t>(eh->e_phnum),
                                                             range, &dyn, nullptr, nullptr))
            return false;

        uint32_t nsyms = sym_count_from_hash_dynsym(dyn.hash, range);
        if (nsyms == 0)
            nsyms =
                sym_count_from_gnu_hash_dynsym<uint32_t>(dyn.gnu_hash, range);
        if (nsyms == 0) return false;

        collect_dynsym_big_funcs_elf<Elf32_Sym>(load_base, dyn, range, nsyms, regions, nregs,
                                                &big_funcs);
    } else {
        return false;
    }

    if (big_funcs.empty()) return false;

    std::sort(big_funcs.begin(), big_funcs.end(),
              [](const std::pair<uintptr_t, size_t> &a, const std::pair<uintptr_t, size_t> &b) {
                  if (a.second != b.second) return a.second > b.second;
                  return a.first < b.first;
              });

    int best_peak = -1;
    uintptr_t best_thunk = 0;
    uintptr_t best_lo = 0;
    uintptr_t best_hi = 0;

    const size_t ntodo = std::min(big_funcs.size(), static_cast<size_t>(kRegDynsymMaxFuncsToHistogram));

    std::unordered_map<uint64_t, int> tgt_counts;
    for (size_t ti = 0; ti < ntodo; ++ti) {
        const uintptr_t f_lo = big_funcs[ti].first;
        const uintptr_t bodysz = big_funcs[ti].second;
        const uintptr_t f_hi = f_lo + bodysz;
        if (f_hi < f_lo || f_lo + UINTPTR_C(4) > f_hi) continue;

        tgt_counts.clear();
        for (uintptr_t addr = f_lo; addr + UINTPTR_C(4) <= f_hi; addr += UINTPTR_C(4)) {
            const uint32_t w = *reinterpret_cast<const uint32_t *>(addr);
            uint64_t tg = UINT64_C(0);
            if (!reg_scan_bl_decode_tgt(addr, w, &tg)) continue;
            tgt_counts[tg]++;
        }

        int func_peak = -1;
        uintptr_t func_thunk = 0;
        for (const auto &kv : tgt_counts) {
            if (kv.second < kRegDynsymBlPeakThreshold) continue;
            const uintptr_t cand_t = static_cast<uintptr_t>(kv.first);
            if (func_peak < 0 || kv.second > func_peak ||
                (kv.second == func_peak && cand_t < func_thunk)) {
                func_peak = kv.second;
                func_thunk = cand_t;
            }
        }
        if (func_peak < kRegDynsymBlPeakThreshold) continue;

        if (best_peak < 0 || func_peak > best_peak ||
            (func_peak == best_peak && func_thunk < best_thunk)) {
            best_peak = func_peak;
            best_thunk = func_thunk;
            best_lo = f_lo;
            best_hi = f_hi;
        }
    }

    if (best_peak < kRegDynsymBlPeakThreshold) return false;

    out->va_start = best_lo;
    out->va_end = best_hi;
    out->thunk_va = best_thunk;
    out->bl_to_thunk_count = best_peak;
    return true;
}

#else

inline bool find_registration_func_from_dynsym(const MappedRegion *, int,
                                               uintptr_t, DynsymCandidate *) {
    return false;
}

#endif

}  // namespace detail
}  // namespace il2cpp_api
