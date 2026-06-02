// Public IL2CPP dynamic API resolver facade (header-only).
#pragma once

#include <cstring>
#include <link.h>
#include <mutex>
#include <unordered_map>

#include "Logger.h"
#include "xdl.h"
#include "API/detail/common.hpp"
#include "API/detail/dynsym_resolve.hpp"
#include "API/detail/xdl_resolve.hpp"

#if defined(__aarch64__)
#include "API/detail/scanner_arm64.hpp"
#elif defined(__arm__)
#include "API/detail/scanner_arm32.hpp"
#endif

namespace il2cpp_api {

inline ApiList resolve_il2cpp_api(void *handle, uintptr_t base) {
    SDKLOGI("[il2cpp_api] resolve start handle=%p base=0x%llx", handle, JLOG_HEX(base));

    ApiList xdl{};
    detail::resolve_xdl_known(handle, xdl);
    SDKLOGI("[il2cpp_api] xdl_known: %zu entries", xdl.size());

    ApiList dyn = detail::resolve_from_dynsym(handle, base);
    SDKLOGI("[il2cpp_api] dynsym: %zu entries", dyn.size());

#if defined(__aarch64__)
    ApiList scan = detail::resolve_stripped_aarch64_manual(handle, base);
    SDKLOGI("[il2cpp_api] scanner_arm64: %zu entries", scan.size());
#elif defined(__arm__)
    ApiList scan = detail::resolve_stripped_arm32_manual(handle, base);
    SDKLOGI("[il2cpp_api] scanner_arm32: %zu entries", scan.size());
#else
    ApiList scan{};
    SDKLOGI("[il2cpp_api] scanner: %zu entries", scan.size());
#endif

    std::unordered_map<std::string, uintptr_t> seen;
    seen.reserve(xdl.size() + dyn.size() + scan.size() + 8);
    ApiList merged;
    merged.reserve(xdl.size() + dyn.size() + scan.size());
    for (auto &e : xdl) detail::dedup_push(merged, seen, e.first, e.second);
    for (auto &e : dyn) detail::dedup_push(merged, seen, e.first, e.second);
    for (auto &e : scan) detail::dedup_push(merged, seen, e.first, e.second);
    SDKLOGI("[il2cpp_api] merged: %zu entries", merged.size());
    return merged;
}

// Convenience overload: tự discover libil2cpp.so handle + base qua xDL.
// Trả về ApiList rỗng nếu libil2cpp.so chưa load.
inline ApiList resolve_il2cpp_api() {
    void *handle = xdl_open("libil2cpp.so", XDL_DEFAULT);
    if (!handle) {
        SDKLOGW("[il2cpp_api] libil2cpp.so not loaded yet");
        return {};
    }

    struct PhdrCtx {
        const char *name;
        uintptr_t base;
    } ctx{"libil2cpp.so", 0};

    xdl_iterate_phdr(
        [](struct dl_phdr_info *info, size_t, void *data) -> int {
            auto *c = static_cast<PhdrCtx *>(data);
            if (info->dlpi_name && std::strstr(info->dlpi_name, c->name)) {
                c->base = static_cast<uintptr_t>(info->dlpi_addr);
                return 1;
            }
            return 0;
        },
        &ctx, XDL_DEFAULT);

    if (ctx.base == 0) {
        SDKLOGW("[il2cpp_api] libil2cpp.so base not found via dl_iterate_phdr");
        xdl_close(handle);
        return {};
    }

    ApiList out = resolve_il2cpp_api(handle, ctx.base);
    xdl_close(handle);
    return out;
}

namespace detail {
inline std::unordered_map<std::string, uintptr_t> &cached_api_map() {
    static std::unordered_map<std::string, uintptr_t> m;
    return m;
}
inline std::mutex &cached_api_mutex() {
    static std::mutex mx;
    return mx;
}
}  // namespace detail

// Lookup pointer của một IL2CPP API theo tên (vd "il2cpp_init").
// Cache lazy: resolve khi gọi lần đầu; nếu libil2cpp.so chưa load (map rỗng)
// thì sẽ RETRY ở lần gọi sau (không cache empty kết quả).
// Trả nullptr nếu chưa load libil2cpp.so hoặc tên không tồn tại.
inline void *get_il2cpp(const char *name) {
    if (!name) return nullptr;
    std::lock_guard<std::mutex> lk(detail::cached_api_mutex());
    auto &m = detail::cached_api_map();
    if (m.empty()) {
        ApiList list = resolve_il2cpp_api();
        if (!list.empty()) {
            m.reserve(list.size());
            for (auto &e : list) m.emplace(e.first, e.second);
        }
        // list rỗng → không cache → lần sau retry.
    }
    auto it = m.find(name);
    if (it == m.end()) return nullptr;
    return reinterpret_cast<void *>(it->second);
}

}  // namespace il2cpp_api
