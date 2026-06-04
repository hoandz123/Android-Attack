#pragma once

#include <cstddef>
#include <cstdint>

namespace lienquan {
namespace HeroIcon {

/** Embedded portrait (JPEG/PNG). Decode: stbi_load_from_memory(iconBytes, iconSize, ...). */
struct Entry {
    const char *fieldName;
    const char *displayName;
    const char *slug;
    const uint8_t *iconBytes;
    size_t iconSize;
};

inline constexpr size_t kHeroCount = 128;

extern const Entry kAll[kHeroCount];

const Entry *FindByDisplayName(const char *displayName) noexcept;
const Entry *FindBySlug(const char *slug) noexcept;
const Entry *FindByFieldName(const char *fieldName) noexcept;

} // namespace HeroIcon
} // namespace lienquan
