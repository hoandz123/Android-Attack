#pragma once

#include <imgui.h>

namespace modui {

/** Upload file/URL → ImTextureID trên render thread (GL context active). `path == nullptr` xóa texture. */
ImTextureID menu_icon_texture(const char *path);

/** Tải URL → `save_path` (thread-safe, không gọi GL). Trả true nếu file sẵn sàng. */
bool menu_icon_download(const char *url, const char *save_path);

} // namespace modui
