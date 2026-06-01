#include "Icon.hpp"

#include <FileManager.hpp>
#include <HttpClient.hpp>
#include <GLES3/gl3.h>
#include <imgui.h>
#include <atomic>
#include <cstring>
#include <thread>

#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace modui {

namespace {

constexpr const char *kFabIconUrl = "https://files.catbox.moe/vpu7cc.png";
constexpr const char *kFabIconPath = "/data/user/0/com.android.attack/fab.png";

std::atomic<bool> g_fab_downloading{false};
std::atomic<bool> g_fab_file_ready{false};
ImTextureID g_fab_icon = (ImTextureID) 0;

ImTextureID LoadIcon(const char *path) {
    static char cached[8192]{};
    static GLuint tex = 0;

    if (!path) {
        if (tex) glDeleteTextures(1, &tex);
        tex = 0;
        cached[0] = '\0';
        return (ImTextureID) 0;
    }
    if (!path[0]) return (ImTextureID) 0;
    if (tex && strncmp(cached, path, sizeof(cached)) == 0) return (ImTextureID) (intptr_t) tex;

    int w = 0, h = 0, ch = 0;
    unsigned char *px = stbi_load(path, &w, &h, &ch, 4);
    if (!px || w <= 0 || h <= 0) {
        if (px) stbi_image_free(px);
        return (ImTextureID) 0;
    }

    if (tex) glDeleteTextures(1, &tex);
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
#ifdef GL_UNPACK_ROW_LENGTH
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
    stbi_image_free(px);

    strncpy(cached, path, sizeof(cached) - 1);
    cached[sizeof(cached) - 1] = '\0';
    return (ImTextureID) (intptr_t) tex;
}

bool DownloadIcon(const char *url, const char *save_path) {
    if (!url || !url[0] || !save_path || !save_path[0]) return false;
    if (fs::IsFile(save_path)) return true;
    const std::string dir = fs::Dirname(save_path);
    if (!dir.empty()) fs::MkdirP(dir);
    if (!http::Download(url, save_path, 20).ok()) return false;
    return fs::IsFile(save_path);
}

void PollFabDownload() {
    if (g_fab_icon || g_fab_file_ready.load(std::memory_order_acquire)) return;
    bool expected = false;
    if (!g_fab_downloading.compare_exchange_strong(expected, true)) return;
    std::thread([] {
        const bool ok = DownloadIcon(kFabIconUrl, kFabIconPath);
        g_fab_file_ready.store(ok, std::memory_order_release);
        g_fab_downloading.store(false, std::memory_order_release);
    }).detach();
}

} // namespace

ImTextureID GetFabIcon() {
    if (!g_fab_icon) {
        PollFabDownload();
        if (g_fab_file_ready.load(std::memory_order_acquire)) g_fab_icon = LoadIcon(kFabIconPath);
    }
    return g_fab_icon;
}

void DrawFabIcon(ImDrawList *dl, const ImVec2 &p0, const ImVec2 &p1, float rounding) {
    if (!dl) return;
    const ImTextureID tex = GetFabIcon();
    if (tex != (ImTextureID) 0) {
        dl->AddImageRounded(tex, p0, p1, ImVec2(0, 0), ImVec2(1, 1), IM_COL32_WHITE, rounding, ImDrawFlags_RoundCornersAll);
    } else {
        dl->AddRectFilled(p0, p1, IM_COL32(0, 0, 0, 255), rounding);
    }
}

} // namespace modui
