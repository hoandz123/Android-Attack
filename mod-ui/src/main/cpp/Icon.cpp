#include "Icon.hpp"

#include <FileManager.hpp>
#include <HttpClient.hpp>
#include <GLES3/gl3.h>
#include <android/log.h>
#include <imgui.h>
#include <atomic>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace modui {

namespace {

constexpr const char *kTag = "AttackIcon";
constexpr const char *kFabIconUrl = "https://tools-mod.com/storage/brand/logo.png";
constexpr const char *kFabIconPath = "/data/user/0/com.android.attack/fab.png";

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, kTag, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, kTag, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, kTag, __VA_ARGS__)

std::atomic<bool> g_fab_downloading{false};
std::atomic<bool> g_fab_file_ready{false};
ImTextureID g_fab_icon = (ImTextureID) 0;
GLuint g_load_icon_tex = 0;
char g_load_icon_path[8192]{};

void DropLoadIconCache() {
    g_load_icon_tex = 0;
    g_load_icon_path[0] = '\0';
}

ImTextureID LoadIcon(const char *path) {
    if (!path) {
        if (g_load_icon_tex) glDeleteTextures(1, &g_load_icon_tex);
        DropLoadIconCache();
        LOGI("LoadIcon cleared cached texture");
        return (ImTextureID) 0;
    }
    if (!path[0]) return (ImTextureID) 0;
    if (g_load_icon_tex && strncmp(g_load_icon_path, path, sizeof(g_load_icon_path)) == 0) {
        LOGI("LoadIcon cache hit path=%s tex=%u", path, g_load_icon_tex);
        return (ImTextureID) (intptr_t) g_load_icon_tex;
    }

    LOGI("LoadIcon stbi_load begin path=%s size=%lld", path, (long long) fs::FileSize(path));
    int w = 0, h = 0, ch = 0;
    unsigned char *px = stbi_load(path, &w, &h, &ch, 4);
    if (!px || w <= 0 || h <= 0) {
        LOGE("LoadIcon stbi_load FAIL path=%s w=%d h=%d reason=%s", path, w, h, stbi_failure_reason() ? stbi_failure_reason() : "?");
        if (px) stbi_image_free(px);
        return (ImTextureID) 0;
    }

    if (g_load_icon_tex) glDeleteTextures(1, &g_load_icon_tex);
    glGenTextures(1, &g_load_icon_tex);
    glBindTexture(GL_TEXTURE_2D, g_load_icon_tex);
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

    strncpy(g_load_icon_path, path, sizeof(g_load_icon_path) - 1);
    g_load_icon_path[sizeof(g_load_icon_path) - 1] = '\0';
    LOGI("LoadIcon OK path=%s %dx%d ch=%d tex=%u", path, w, h, ch, g_load_icon_tex);
    return (ImTextureID) (intptr_t) g_load_icon_tex;
}

std::string MetaPath(const char *save_path) { return std::string(save_path) + ".etag"; }

bool ReadMeta(const std::string &meta_path, http::CacheValidators *out) {
    if (!out || !fs::IsFile(meta_path)) return false;
    const std::vector<uint8_t> raw = fs::ReadBytes(meta_path);
    if (raw.empty()) return false;
    const char *data = reinterpret_cast<const char *>(raw.data());
    const size_t len = raw.size();
    size_t i = 0;
    auto next_line = [&]() -> std::string {
        std::string line;
        while (i < len && data[i] != '\n' && data[i] != '\r') line += data[i++];
        while (i < len && (data[i] == '\n' || data[i] == '\r')) ++i;
        return line;
    };
    out->etag = next_line();
    out->last_modified = next_line();
    return !out->etag.empty() || !out->last_modified.empty();
}

void WriteMeta(const std::string &meta_path, const http::CacheValidators &v) {
    const std::string content = v.etag + "\n" + v.last_modified + "\n";
    const auto wr = fs::WriteBytes(meta_path, content.data(), content.size());
    if (!wr.ok()) LOGW("WriteMeta failed path=%s: %s", meta_path.c_str(), wr.error.c_str());
}

bool DownloadIcon(const char *url, const char *save_path) {
    if (!url || !url[0] || !save_path || !save_path[0]) {
        LOGE("DownloadIcon invalid args url=%p path=%p", (void *) url, (void *) save_path);
        return false;
    }
    LOGI("DownloadIcon begin url=%s path=%s", url, save_path);
    const int64_t existing = fs::IsFile(save_path) ? fs::FileSize(save_path) : -1;
    LOGI("DownloadIcon existing size=%lld", (long long) existing);
    if (existing == 0) {
        LOGW("DownloadIcon removing stale 0-byte file path=%s", save_path);
        fs::Remove(save_path);
        fs::Remove(MetaPath(save_path));
    }
    const std::string dir = fs::Dirname(save_path);
    if (!dir.empty()) {
        const auto mk = fs::MkdirP(dir);
        if (!mk.error.empty()) LOGW("DownloadIcon MkdirP %s: %s", dir.c_str(), mk.error.c_str());
    }
    const std::string meta_path = MetaPath(save_path);
    http::CacheValidators in;
    http::CacheValidators out;
    const bool have_cache = fs::IsFile(save_path) && fs::FileSize(save_path) > 0;
    const bool have_meta = ReadMeta(meta_path, &in);
    const http::CacheValidators *in_ptr = nullptr;
    if (have_cache && have_meta && (!in.etag.empty() || !in.last_modified.empty())) {
        in_ptr = &in;
        LOGI("DownloadIcon conditional GET if-none-match=%s if-modified-since=%s", in.etag.empty() ? "(none)" : in.etag.c_str(), in.last_modified.empty() ? "(none)" : in.last_modified.c_str());
    } else {
        LOGI("DownloadIcon plain GET (cache=%d meta=%d)", have_cache ? 1 : 0, have_meta ? 1 : 0);
    }
    const http::Response resp = http::DownloadConditional(url, save_path, 20, in_ptr, &out);
    const int64_t after = fs::IsFile(save_path) ? fs::FileSize(save_path) : -1;
    if (resp.cache_hit()) {
        LOGI("DownloadIcon conditional -> 304 keep cache size=%lld", (long long) after);
        return after > 0;
    }
    if (resp.ok()) {
        WriteMeta(meta_path, out);
        DropLoadIconCache();
        g_fab_icon = (ImTextureID) 0;
        LOGI("DownloadIcon conditional -> 200 replaced (%lld bytes)", (long long) after);
        return after > 0;
    }
    LOGI("DownloadIcon http ok=%d status=%ld error=%s size_after=%lld", resp.ok() ? 1 : 0, resp.status, resp.error.empty() ? "(none)" : resp.error.c_str(), (long long) after);
    LOGW("DownloadIcon failed — removing path=%s", save_path);
    fs::Remove(save_path);
    fs::Remove(meta_path);
    return false;
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

void InvalidateIcon() {
    LOGI("InvalidateIcon: dropping stale texture for context recreate");
    g_fab_icon = (ImTextureID) 0;
    DropLoadIconCache();
}

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
