// Preferred path: resolve known IL2CPP entry points via explicit `xdl_sym` lookups.
#pragma once

#include "API/detail/common.hpp"

#include <cstddef>
#include <unordered_map>

#include "Logger.h"
#include "xdl.h"

namespace il2cpp_api {
namespace detail {

inline void resolve_xdl_known(void *handle, ApiList &out) {
    if (!handle) return;

    ApiList scratch;
    scratch.reserve(sizeof(kKnownApiNames) / sizeof(kKnownApiNames[0]));
    std::unordered_map<std::string, uintptr_t> seen;

    seen.reserve(scratch.capacity());
    const int total = static_cast<int>(sizeof(kKnownApiNames) / sizeof(kKnownApiNames[0]));
    int missing = 0;
    for (const char *name : kKnownApiNames) {
        void *sym = xdl_sym(handle, name, nullptr);
        if (sym) {
            SDKLOGD("[xdl] %s -> %p", name, sym);
            dedup_push(scratch, seen, std::string(name), reinterpret_cast<uintptr_t>(sym));
        } else {
            ++missing;
        }
    }
    SDKLOGD("[xdl] missing %d/%d", missing, total);

    std::swap(out, scratch);
}

}  // namespace detail
}  // namespace il2cpp_api
