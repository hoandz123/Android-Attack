#include "Icon.hpp"

#include <FileManager.hpp>
#include <HttpClient.hpp>
#include <GLES3/gl3.h>
#include <android/log.h>
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

constexpr const char *kTag = "AttackIcon";
constexpr const char *kFabIconUrl = "https://files.catbox.moe/vpu7cc.png";
constexpr const char *kFabIconPath = "/data/user/0/com.android.attack/fab.png";

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, kTag, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, kTag, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, kTag, __VA_ARGS__)

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
        LOGI("LoadIcon cleared cached texture");
        return (ImTextureID) 0;
    }
    if (!path[0]) return (ImTextureID) 0;
    if (tex && strncmp(cached, path, sizeof(cached)) == 0) {
        LOGI("LoadIcon cache hit path=%s tex=%u", path, tex);
        return (ImTextureID) (intptr_t) tex;
    }

    LOGI("LoadIcon stbi_load begin path=%s size=%lld", path, (long long) fs::FileSize(path));
    int w = 0, h = 0, ch = 0;
    unsigned char *px = stbi_load(path, &w, &h, &ch, 4);
    if (!px || w <= 0 || h <= 0) {
        LOGE("LoadIcon stbi_load FAIL path=%s w=%d h=%d reason=%s", path, w, h, stbi_failure_reason() ? stbi_failure_reason() : "?");
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
    LOGI("LoadIcon OK path=%s %dx%d ch=%d tex=%u", path, w, h, ch, tex);
    return (ImTextureID) (intptr_t) tex;
}

bool DownloadIcon(const char *url, const char *save_path) {
    if (!url || !url[0] || !save_path || !save_path[0]) {
        LOGE("DownloadIcon invalid args url=%p path=%p", (void *) url, (void *) save_path);
        return false;
    }
    LOGI("DownloadIcon begin url=%s path=%s", url, save_path);
    const int64_t existing = fs::IsFile(save_path) ? fs::FileSize(save_path) : -1;
    LOGI("DownloadIcon existing size=%lld", (long long) existing);
    if (existing > 0) {
        LOGI("DownloadIcon skip download (valid file %lld bytes)", (long long) existing);
        return true;
    }
    if (existing == 0) {
        LOGW("DownloadIcon removing stale 0-byte file path=%s", save_path);
        fs::Remove(save_path);
    }
    const std::string dir = fs::Dirname(save_path);
    if (!dir.empty()) {
        const auto mk = fs::MkdirP(dir);
        if (!mk.error.empty()) LOGW("DownloadIcon MkdirP %s: %s", dir.c_str(), mk.error.c_str());
    }
    const http::Response resp = http::Download(url, save_path, 20);
    const int64_t after = fs::IsFile(save_path) ? fs::FileSize(save_path) : -1;
    LOGI("DownloadIcon http ok=%d status=%ld error=%s size_after=%lld", resp.ok() ? 1 : 0, resp.status, resp.error.empty() ? "(none)" : resp.error.c_str(), (long long) after);
    if (!resp.ok() || after <= 0) {
        LOGW("DownloadIcon failed — removing path=%s", save_path);
        fs::Remove(save_path);
        return false;
    }
    LOGI("DownloadIcon success path=%s bytes=%lld", save_path, (long long) after);
    return true;
}

void PollFabDownload() {
    if (g_fab_icon || g_fab_file_ready.load(std::memory_order_acquire)) return;
    bool expected = false;
    if (!g_fab_downloading.compare_exchange_strong(expected, true)) {
        LOGI("PollFabDownload skip (download already in progress)");
        return;
    }
    LOGI("PollFabDownload spawning download thread url=%s path=%s", kFabIconUrl, kFabIconPath);
    std::thread([] {
        LOGI("FabDownload thread started");
        const bool ok = DownloadIcon(kFabIconUrl, kFabIconPath);
        g_fab_file_ready.store(ok, std::memory_order_release);
        g_fab_downloading.store(false, std::memory_order_release);
        LOGI("FabDownload thread done ok=%d file_ready=%d size=%lld", ok ? 1 : 0, ok ? 1 : 0, (long long) (fs::IsFile(kFabIconPath) ? fs::FileSize(kFabIconPath) : -1));
    }).detach();
}

} // namespace

ImTextureID GetFabIcon() {
    if (!g_fab_icon) {
        PollFabDownload();
        if (g_fab_file_ready.load(std::memory_order_acquire)) {
            LOGI("GetFabIcon loading texture from %s", kFabIconPath);
            g_fab_icon = LoadIcon(kFabIconPath);
            if (!g_fab_icon) {
                LOGW("GetFabIcon LoadIcon failed — removing %s and retry later", kFabIconPath);
                fs::Remove(kFabIconPath);
                g_fab_file_ready.store(false, std::memory_order_release);
            } else {
                LOGI("GetFabIcon texture ready id=%p", (void *) g_fab_icon);
            }
        }
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
