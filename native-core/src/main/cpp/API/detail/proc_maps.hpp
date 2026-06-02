// `/proc/self/maps` helpers for locating `libil2cpp.so` segments (VMA, file offsets, RX).
#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "Logger.h"

namespace il2cpp_api {
namespace detail {

struct MappedRegion {
    uintptr_t va_start = 0;
    uintptr_t va_end = 0;
    uintptr_t file_off = 0;
    size_t len = 0;
    bool is_exec = false;
};

inline bool ends_with_path(const std::string &s, const char *suffix) {
    const size_t lf = std::strlen(suffix);
    return s.size() >= lf &&
           std::memcmp(s.data() + s.size() - lf, suffix, lf) == 0;
}

inline bool parse_maps_line_il2cpp(const char *line, uintptr_t &start, uintptr_t &end,
                                   char (&perms)[8], uintptr_t &foff,
                                   std::string &path_out) {
    const char *pslash = std::strchr(line, '/');
    if (!pslash) return false;

    std::string head(line, pslash - line);
    unsigned long r_start = 0;
    unsigned long r_end = 0;
    unsigned long r_foff = 0;
    char pbuf[16] = {};
    if (std::sscanf(head.c_str(), "%lx-%lx %8s %lx", &r_start, &r_end, pbuf,
                    &r_foff) != 4) {
        return false;
    }
    path_out.assign(pslash);
    const char *nl = strchr(path_out.c_str(), '\n');
    if (nl) path_out.erase(static_cast<size_t>(nl - path_out.data()));

    std::strncpy(perms, pbuf, sizeof(perms) - 1);
    perms[sizeof(perms) - 1] = '\0';
    start = static_cast<uintptr_t>(r_start);
    end = static_cast<uintptr_t>(r_end);
    foff = static_cast<uintptr_t>(r_foff);
    return true;
}

inline bool read_libil2cpp_regions(std::vector<MappedRegion> &out) {
    out.clear();

    constexpr int k_maps_line_max = 1024;
    constexpr size_t k_max_raw = 256;

    FILE *mf = std::fopen("/proc/self/maps", "re");
    if (!mf) return false;

    std::vector<MappedRegion> raw;
    raw.reserve(32);

    char line[k_maps_line_max];
    while (std::fgets(line, sizeof(line), mf)) {
        uintptr_t s = 0;
        uintptr_t e = 0;
        uintptr_t fo = 0;
        char perms[8] = {};
        std::string path;
        if (!parse_maps_line_il2cpp(line, s, e, perms, fo, path)) continue;
        if (!std::strchr(perms, 'r')) continue;
        if (!ends_with_path(path, "libil2cpp.so")) continue;

        MappedRegion rg{};
        rg.va_start = s;
        rg.va_end = e;
        rg.file_off = fo;
        rg.len = static_cast<size_t>(e - s);
        if (rg.len == 0) continue;
        rg.is_exec = std::strchr(perms, 'x') || std::strchr(perms, 'X');

        bool dup = false;
        for (const MappedRegion &p : raw) {
            if (p.file_off == rg.file_off && p.len == rg.len) {
                dup = true;
                break;
            }
        }
        if (dup) continue;
        if (raw.size() >= k_max_raw) {
            std::fclose(mf);
            return false;
        }
        raw.push_back(rg);
    }
    std::fclose(mf);

    out.reserve(raw.size());
    for (size_t a = 0; a < raw.size(); ++a) {
        bool nest_smaller = false;
        for (size_t b = 0; b < raw.size(); ++b) {
            if (a == b) continue;
            if (raw[b].file_off == raw[a].file_off && raw[b].len > raw[a].len) {
                nest_smaller = true;
                break;
            }
        }
        if (!nest_smaller) out.push_back(raw[a]);
    }
    std::sort(out.begin(), out.end(), [](const MappedRegion &x, const MappedRegion &y) {
        if (x.va_start != y.va_start) return x.va_start < y.va_start;
        return x.file_off < y.file_off;
    });
    return !out.empty();
}

inline bool addr_in_any_region(const std::vector<MappedRegion> &regs, uintptr_t va,
                               bool require_exec = false) {
    for (const MappedRegion &r : regs) {
        if (require_exec && !r.is_exec) continue;
        if (va >= r.va_start && va < r.va_end) return true;
    }
    return false;
}

inline bool addr_in_any_region(const MappedRegion *regs, int nregs, uintptr_t va,
                               bool require_exec = false) {
    if (!regs || nregs <= 0) return false;
    for (int i = 0; i < nregs; ++i) {
        const MappedRegion &r = regs[i];
        if (require_exec && !r.is_exec) continue;
        if (va >= r.va_start && va < r.va_end) return true;
    }
    return false;
}

inline bool memcpy_from_regions(const MappedRegion *segs, int nseg, uintptr_t va,
                                void *dst, size_t len) {
    auto *d = static_cast<unsigned char *>(dst);
    size_t off = 0;
    while (off < len) {
        uintptr_t a = va + off;
        int found = -1;
        for (int i = 0; i < nseg; ++i) {
            if (a >= segs[i].va_start && a < segs[i].va_end) {
                found = i;
                break;
            }
        }
        if (found < 0) return false;
        size_t seg_room = static_cast<size_t>(segs[found].va_end - a);
        size_t n = len - off;
        if (n > seg_room) n = seg_room;
        std::memcpy(d + off, reinterpret_cast<void *>(a), n);
        off += n;
    }
    return true;
}

}  // namespace detail
}  // namespace il2cpp_api
