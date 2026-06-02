// Fast-path over `.dynsym` using libc `<elf.h>` only (no libelf helpers).
#pragma once

#include <cstdint>

#include <unordered_map>
#include <vector>

#include <elf.h>

#include "Logger.h"
#include "API/detail/common.hpp"
#include "API/detail/proc_maps.hpp"

#include "xdl.h"

namespace il2cpp_api {
namespace detail {

struct ModuleRangeDynsym {
    uintptr_t lo = UINTPTR_MAX;
    uintptr_t hi = 0;
};

template <typename PhdrT>
inline ModuleRangeDynsym calc_module_range_dynsym(uintptr_t base, const PhdrT *phdrs,
                                                  size_t phnum) {
    ModuleRangeDynsym r{};
    for (size_t i = 0; i < phnum; ++i) {
        if (phdrs[i].p_type != PT_LOAD) continue;
        uintptr_t seg_lo = base + static_cast<uintptr_t>(phdrs[i].p_vaddr);
        uintptr_t seg_hi =
            seg_lo + static_cast<uintptr_t>(phdrs[i].p_memsz ? phdrs[i].p_memsz
                                                             : phdrs[i].p_filesz);
        if (seg_lo < r.lo) r.lo = seg_lo;
        if (seg_hi > r.hi) r.hi = seg_hi;
    }
    if (r.lo == UINTPTR_MAX || r.hi <= r.lo) {
        r.lo = 0;
        r.hi = 0;
    }
    return r;
}

inline uintptr_t dyn_ptr_to_va_dynsym(uintptr_t raw, uintptr_t base,
                                      const ModuleRangeDynsym &range) {
    if (raw >= range.lo && raw < range.hi) return raw;
    if (raw > UINTPTR_MAX - base) return 0;
    uintptr_t rebased = base + raw;
    if (rebased >= range.lo && rebased < range.hi) return rebased;
    return 0;
}

struct DynSymInfoDynsym {
    uintptr_t strtab = 0;
    uintptr_t symtab = 0;
    uintptr_t hash = 0;
    uintptr_t gnu_hash = 0;
    size_t strsz = 0;
    size_t syment = 0;
};

template <typename PhdrT, typename DynT>
inline bool read_dynamic_info_dynsym(uintptr_t base, const PhdrT *phdrs, size_t phnum,
                                     const ModuleRangeDynsym &range, DynSymInfoDynsym *out,
                                     uintptr_t *out_dyn_vma = nullptr,
                                     size_t *out_dyn_nslots = nullptr) {
    uintptr_t dyn_addr = 0;
    size_t dyn_cnt = 0;
    for (size_t i = 0; i < phnum; ++i) {
        if (phdrs[i].p_type != PT_DYNAMIC) continue;
        dyn_addr = base + static_cast<uintptr_t>(phdrs[i].p_vaddr);
        dyn_cnt = static_cast<size_t>(phdrs[i].p_memsz / sizeof(DynT));
        break;
    }
    if (dyn_addr == 0 || dyn_cnt == 0) {
        SDKLOGW("[dynsym] fail: PT_DYNAMIC missing or empty (dyn_addr=0x%llx dyn_cnt=%zu)",
             JLOG_HEX(dyn_addr), static_cast<size_t>(dyn_cnt));
        return false;
    }

    if (out_dyn_vma) *out_dyn_vma = dyn_addr;
    if (out_dyn_nslots) *out_dyn_nslots = dyn_cnt;

    uintptr_t strtab_raw = 0;
    uintptr_t symtab_raw = 0;
    uintptr_t hash_raw = 0;
    uintptr_t gnu_hash_raw = 0;
    size_t strsz = 0;
    size_t syment = 0;

    const DynT *dyn = reinterpret_cast<const DynT *>(dyn_addr);
    for (size_t i = 0; i < dyn_cnt; ++i) {
        if (dyn[i].d_tag == DT_NULL) break;
        switch (dyn[i].d_tag) {
            case DT_STRTAB:
                strtab_raw = static_cast<uintptr_t>(dyn[i].d_un.d_ptr);
                break;
            case DT_SYMTAB:
                symtab_raw = static_cast<uintptr_t>(dyn[i].d_un.d_ptr);
                break;
            case DT_STRSZ: strsz = static_cast<size_t>(dyn[i].d_un.d_val); break;
            case DT_SYMENT: syment = static_cast<size_t>(dyn[i].d_un.d_val); break;
            case DT_HASH: hash_raw = static_cast<uintptr_t>(dyn[i].d_un.d_ptr); break;
            case DT_GNU_HASH:
                gnu_hash_raw = static_cast<uintptr_t>(dyn[i].d_un.d_ptr);
                break;
            default: break;
        }
    }

    out->strtab = dyn_ptr_to_va_dynsym(strtab_raw, base, range);
    out->symtab = dyn_ptr_to_va_dynsym(symtab_raw, base, range);
    out->hash = dyn_ptr_to_va_dynsym(hash_raw, base, range);
    out->gnu_hash = dyn_ptr_to_va_dynsym(gnu_hash_raw, base, range);
    out->strsz = strsz;
    out->syment = syment;

    if (out->strtab == 0 || out->symtab == 0 || out->strsz == 0 || out->syment == 0) {
        SDKLOGW("[dynsym] fail: dynamic table incomplete "
              "(strtab=0x%llx symtab=0x%llx strsz=%zu syment=%zu hash=0x%llx gnu_hash=0x%llx)",
             JLOG_HEX(out->strtab), JLOG_HEX(out->symtab), out->strsz, out->syment,
             JLOG_HEX(out->hash), JLOG_HEX(out->gnu_hash));
        return false;
    }
    return true;
}

template <typename WordT>
inline uint32_t sym_count_from_gnu_hash_dynsym(uintptr_t gnu_hash,
                                               const ModuleRangeDynsym &range) {
    if (gnu_hash == 0 || gnu_hash + 16 >= range.hi) return 0;
    const uint32_t *hdr = reinterpret_cast<const uint32_t *>(gnu_hash);
    uint32_t nbuckets = hdr[0];
    uint32_t symoffset = hdr[1];
    uint32_t bloom_size = hdr[2];

    uintptr_t p = gnu_hash + 16;
    size_t bloom_bytes = static_cast<size_t>(bloom_size) * sizeof(WordT);
    if (p + bloom_bytes >= range.hi) return 0;
    p += bloom_bytes;
    if (p + static_cast<size_t>(nbuckets) * sizeof(uint32_t) >= range.hi) return 0;

    const uint32_t *buckets = reinterpret_cast<const uint32_t *>(p);
    const uint32_t *chains = buckets + nbuckets;
    uint32_t max_bucket = 0;
    for (uint32_t i = 0; i < nbuckets; ++i) {
        if (buckets[i] > max_bucket) max_bucket = buckets[i];
    }
    if (max_bucket < symoffset) return symoffset;

    uint32_t idx = max_bucket;
    for (uint32_t guard = 0; guard < (1u << 24); ++guard) {
        uintptr_t cptr = reinterpret_cast<uintptr_t>(chains + (idx - symoffset));
        if (cptr + sizeof(uint32_t) >= range.hi) return 0;
        uint32_t c = chains[idx - symoffset];
        idx++;
        if (c & 1u) return idx;
    }
    return 0;
}

inline uint32_t sym_count_from_hash_dynsym(uintptr_t hash, const ModuleRangeDynsym &range) {
    if (hash == 0 || hash + 8 >= range.hi) return 0;
    const uint32_t *tbl = reinterpret_cast<const uint32_t *>(hash);
    return tbl[1];
}

template <typename SymT>
inline uintptr_t resolve_sym_value_dynsym(uintptr_t base, uintptr_t value,
                                          const ModuleRangeDynsym &range) {
    if (value >= range.lo && value < range.hi) return value;
    if (value > UINTPTR_MAX - base) return 0;
    uintptr_t rebased = base + value;
    if (rebased >= range.lo && rebased < range.hi) return rebased;
    return 0;
}

template <typename SymT>
inline void collect_dyn_symbols_dynsym(uintptr_t base, const DynSymInfoDynsym &dyn,
                                       const ModuleRangeDynsym &range, uint32_t nsyms,
                                       ApiList *out,
                                       std::unordered_map<std::string, uintptr_t> *seen,
                                       const std::vector<MappedRegion> &il2_maps,
                                       size_t *sym_scanned_out = nullptr,
                                       size_t *il2cpp_matched_out = nullptr) {
    if (nsyms == 0) return;

    size_t max_possible =
        dyn.syment > 0 ? static_cast<size_t>((range.hi - dyn.symtab) / dyn.syment) : 0;
    if (max_possible == 0) {
        SDKLOGW("[dynsym] fail: symtab slice empty (symtab=0x%llx syment=%zu range_hi=0x%llx)",
             JLOG_HEX(dyn.symtab), dyn.syment, JLOG_HEX(range.hi));
        return;
    }
    if (nsyms > max_possible) nsyms = static_cast<uint32_t>(max_possible);

    for (uint32_t i = 0; i < nsyms; ++i) {
        if (sym_scanned_out) ++*sym_scanned_out;

        uintptr_t sym_addr = dyn.symtab + static_cast<uintptr_t>(i) * dyn.syment;
        if (sym_addr + sizeof(SymT) >= range.hi) break;
        const SymT *sym = reinterpret_cast<const SymT *>(sym_addr);

        if (sym->st_name >= dyn.strsz || sym->st_value == 0) continue;

        unsigned st_type = 0;
        if constexpr (sizeof(SymT) == sizeof(Elf64_Sym)) {
            st_type = ELF64_ST_TYPE(sym->st_info);
        } else {
            st_type = ELF32_ST_TYPE(sym->st_info);
        }
        if (st_type != STT_FUNC && st_type != STT_NOTYPE) continue;

        uintptr_t ptr = resolve_sym_value_dynsym<SymT>(
            base, static_cast<uintptr_t>(sym->st_value), range);
        if (ptr == 0) continue;

        const char *name = reinterpret_cast<const char *>(dyn.strtab + sym->st_name);
        size_t room = dyn.strsz - sym->st_name;

        if (!read_bounded_cstr(name, room)) continue;
        if (!is_valid_api_name(name)) continue;

        if (!addr_in_any_region(il2_maps, ptr, true)) continue;

        if (il2cpp_matched_out) {
            const size_t before = out->size();
            dedup_push(*out, *seen, std::string(name), ptr);
            if (out->size() > before) ++*il2cpp_matched_out;
        } else {
            dedup_push(*out, *seen, std::string(name), ptr);
        }
    }
}

inline ApiList resolve_from_dynsym(void *handle, uintptr_t il2cpp_base_param) {
    ApiList out;
    if (!handle) {
        SDKLOGW("[dynsym] fail: null handle");
        return out;
    }

    xdl_info_t info{};
    if (xdl_info(handle, XDL_DI_DLINFO, &info) != 0) {
        SDKLOGW("[dynsym] fail: xdl_info(XDL_DI_DLINFO)");
        return out;
    }

    uintptr_t il2cpp_base = il2cpp_base_param;
    if (il2cpp_base == 0) il2cpp_base = reinterpret_cast<uintptr_t>(info.dli_fbase);
    if (il2cpp_base == 0) {
        SDKLOGW("[dynsym] fail: il2cpp base is 0");
        return out;
    }

    const uint8_t *ident = reinterpret_cast<const uint8_t *>(il2cpp_base);
    if (!(ident[EI_MAG0] == ELFMAG0 && ident[EI_MAG1] == ELFMAG1 &&
          ident[EI_MAG2] == ELFMAG2 && ident[EI_MAG3] == ELFMAG3)) {
        SDKLOGW("[dynsym] fail: bad ELF magic at base 0x%llx", JLOG_HEX(il2cpp_base));
        return out;
    }

    std::vector<MappedRegion> il2_regions;
    if (!read_libil2cpp_regions(il2_regions) || il2_regions.empty()) {
        SDKLOGW("[dynsym] fail: no libil2cpp.so regions from maps");
        return out;
    }

    std::unordered_map<std::string, uintptr_t> seen;
    seen.reserve(512);

    if (ident[EI_CLASS] == ELFCLASS64) {
        SDKLOGI("[dynsym] ELF class 64, il2cpp_base=0x%llx", JLOG_HEX(il2cpp_base));

        const Elf64_Ehdr *eh = reinterpret_cast<const Elf64_Ehdr *>(il2cpp_base);
        const Elf64_Phdr *phdrs =
            info.dlpi_phdr ? reinterpret_cast<const Elf64_Phdr *>(info.dlpi_phdr)
                           : reinterpret_cast<const Elf64_Phdr *>(il2cpp_base + eh->e_phoff);
        size_t phnum =
            info.dlpi_phnum ? info.dlpi_phnum : static_cast<size_t>(eh->e_phnum);
        ModuleRangeDynsym range = calc_module_range_dynsym(il2cpp_base, phdrs, phnum);
        if (range.lo == 0 || range.hi == 0) {
            SDKLOGW("[dynsym] fail: empty module VM range (lo=0x%llx hi=0x%llx)", JLOG_HEX(range.lo),
                 JLOG_HEX(range.hi));
            return out;
        }

        DynSymInfoDynsym dyn{};
        uintptr_t dyn_vma = 0;
        size_t dyn_nslots = 0;
        if (!read_dynamic_info_dynsym<Elf64_Phdr, Elf64_Dyn>(il2cpp_base, phdrs, phnum, range,
                                                              &dyn, &dyn_vma, &dyn_nslots))
            return out;

        SDKLOGI("[dynsym] dyn_addr=0x%llx dyn_entries=%zu strtab=0x%llx symtab=0x%llx strsz=%zu "
              "syment=%zu",
             JLOG_HEX(dyn_vma), dyn_nslots, JLOG_HEX(dyn.strtab), JLOG_HEX(dyn.symtab),
             dyn.strsz, dyn.syment);

        uint32_t nsyms = sym_count_from_hash_dynsym(dyn.hash, range);
        if (nsyms == 0)
            nsyms = sym_count_from_gnu_hash_dynsym<uint64_t>(dyn.gnu_hash, range);
        SDKLOGI("[dynsym] nsyms (from hash/gnu_hash)=%u", static_cast<unsigned>(nsyms));

        size_t sym_scanned = 0;
        size_t il2cpp_matched = 0;
        collect_dyn_symbols_dynsym<Elf64_Sym>(il2cpp_base, dyn, range, nsyms, &out, &seen,
                                              il2_regions, &sym_scanned, &il2cpp_matched);
        SDKLOGI("[dynsym] scanned symbols=%zu il2cpp_matched=%zu out_total=%zu", sym_scanned,
             il2cpp_matched, out.size());
    } else if (ident[EI_CLASS] == ELFCLASS32) {
        SDKLOGI("[dynsym] ELF class 32, il2cpp_base=0x%llx", JLOG_HEX(il2cpp_base));

        const Elf32_Ehdr *eh = reinterpret_cast<const Elf32_Ehdr *>(il2cpp_base);
        const Elf32_Phdr *phdrs =
            info.dlpi_phdr ? reinterpret_cast<const Elf32_Phdr *>(info.dlpi_phdr)
                           : reinterpret_cast<const Elf32_Phdr *>(il2cpp_base + eh->e_phoff);
        size_t phnum =
            info.dlpi_phnum ? info.dlpi_phnum : static_cast<size_t>(eh->e_phnum);
        ModuleRangeDynsym range = calc_module_range_dynsym(il2cpp_base, phdrs, phnum);
        if (range.lo == 0 || range.hi == 0) {
            SDKLOGW("[dynsym] fail: empty module VM range (lo=0x%llx hi=0x%llx)", JLOG_HEX(range.lo),
                 JLOG_HEX(range.hi));
            return out;
        }

        DynSymInfoDynsym dyn{};
        uintptr_t dyn_vma = 0;
        size_t dyn_nslots = 0;
        if (!read_dynamic_info_dynsym<Elf32_Phdr, Elf32_Dyn>(il2cpp_base, phdrs, phnum, range,
                                                              &dyn, &dyn_vma, &dyn_nslots))
            return out;

        SDKLOGI("[dynsym] dyn_addr=0x%llx dyn_entries=%zu strtab=0x%llx symtab=0x%llx strsz=%zu "
              "syment=%zu",
             JLOG_HEX(dyn_vma), dyn_nslots, JLOG_HEX(dyn.strtab), JLOG_HEX(dyn.symtab),
             dyn.strsz, dyn.syment);

        uint32_t nsyms = sym_count_from_hash_dynsym(dyn.hash, range);
        if (nsyms == 0)
            nsyms = sym_count_from_gnu_hash_dynsym<uint32_t>(dyn.gnu_hash, range);
        SDKLOGI("[dynsym] nsyms (from hash/gnu_hash)=%u", static_cast<unsigned>(nsyms));

        size_t sym_scanned = 0;
        size_t il2cpp_matched = 0;
        collect_dyn_symbols_dynsym<Elf32_Sym>(il2cpp_base, dyn, range, nsyms, &out, &seen,
                                              il2_regions, &sym_scanned, &il2cpp_matched);
        SDKLOGI("[dynsym] scanned symbols=%zu il2cpp_matched=%zu out_total=%zu", sym_scanned,
             il2cpp_matched, out.size());
    } else {
        SDKLOGW("[dynsym] fail: unsupported ELF class %u", static_cast<unsigned>(ident[EI_CLASS]));
    }

    return out;
}

}  // namespace detail
}  // namespace il2cpp_api
