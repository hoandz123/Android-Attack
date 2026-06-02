// Shared IL2CPP API resolver types & small helpers used by every backend.
#pragma once

#include <cstdint>
#include <regex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <cstring>

#include "Logger.h"

namespace il2cpp_api {

using ApiEntry = std::pair<std::string, uintptr_t>;
using ApiList = std::vector<ApiEntry>;

namespace detail {

inline constexpr const char *kKnownApiNames[] = {
    "il2cpp_init",
    "il2cpp_init_utf16",
    "il2cpp_shutdown",
    "il2cpp_domain_get",
    "il2cpp_domain_get_assemblies",
    "il2cpp_assembly_get_image",
    "il2cpp_class_from_name",
    "il2cpp_class_get_name",
    "il2cpp_class_get_namespace",
    "il2cpp_class_get_methods",
    "il2cpp_class_get_method_from_name",
    "il2cpp_method_get_name",
    "il2cpp_method_get_param_count",
    "il2cpp_method_get_param_name",
    "il2cpp_method_get_return_type",
    "il2cpp_method_get_pointer",
    "il2cpp_object_get_class",
    "il2cpp_object_new",
    "il2cpp_runtime_invoke",
    "il2cpp_runtime_class_init",
    "il2cpp_resolve_icall",
    "il2cpp_add_internal_call",
    "il2cpp_alloc",
    "il2cpp_free",
    "il2cpp_array_new",
    "il2cpp_array_length",
    "il2cpp_array_get_byte_length",
    "il2cpp_string_new",
    "il2cpp_string_new_utf16",
    "il2cpp_thread_attach",
    "il2cpp_thread_detach",
    "il2cpp_exception_from_name_msg",
    "il2cpp_field_get_name",
    "il2cpp_field_get_offset",
    "il2cpp_field_get_type",
    "il2cpp_property_get_name",
    "il2cpp_property_get_get_method",
    "il2cpp_property_get_set_method",
    "il2cpp_type_get_name",
    "il2cpp_type_get_object",
    "il2cpp_value_box",
    "il2cpp_gc_collect",
    "il2cpp_gc_disable",
    "il2cpp_gc_enable",
    "il2cpp_gc_is_disabled",
    "il2cpp_stats_dump_to_file",
    "il2cpp_stats_get_value",
};

inline bool is_valid_api_name(const std::string &name) {
    static const std::regex k_il2cpp_re(R"(^il2cpp_[A-Za-z0-9_]+$)");
    return std::regex_match(name, k_il2cpp_re);
}

inline bool is_valid_api_name(const char *s) {
    if (!s) return false;
    return is_valid_api_name(std::string(s));
}

inline void dedup_push(ApiList &out, std::unordered_map<std::string, uintptr_t> &seen,
                       const std::string &name, uintptr_t ptr) {
    if (name.empty() || ptr == 0) return;
    if (seen.find(name) != seen.end()) return;
    seen.emplace(name, ptr);
    out.emplace_back(name, ptr);
}

inline bool read_bounded_cstr(const char *s, size_t max_len) {
    if (!s || max_len == 0) return false;
    for (size_t i = 0; i < max_len; ++i) {
        char c = s[i];
        if (c == '\0') return i > 0;
    }
    return false;
}

}  // namespace detail
}  // namespace il2cpp_api
