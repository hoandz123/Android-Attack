#pragma once

#include <imgui.h>

namespace modui {

/** `stbi_load` file local → GL texture trên render thread. `path == nullptr` xóa texture. */
ImTextureID LoadIcon(const char *path);

/** Tải URL → `save_path` nếu file chưa có (không GL). Trả true khi file tồn tại. */
bool DownloadIcon(const char *url, const char *save_path);

} // namespace modui
