#include "Icon.hpp"
#include "ModUi.hpp"

#define LOGGER_TAG "ATTACK_Icon"
#include <Includes/Logger.h>

#include <FileManager.hpp>
#include <HttpClient.hpp>
#include <GLES3/gl3.h>
#include <imgui.h>
#include <atomic>
#include <chrono>
#include <cstdio>
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

constexpr int kMaxFabRetries = 5;
constexpr int64_t kFabBackoffMs = 30000;

std::atomic<bool> g_fab_downloading{false};
std::atomic<bool> g_fab_file_ready{false};
std::atomic<int> g_fab_fail_count{0};
std::atomic<int64_t> g_fab_next_retry_ms{0};
ImTextureID g_fab_icon = (ImTextureID) 0;
GLuint g_load_icon_tex = 0;
char g_load_icon_path[8192]{};

int64_t NowMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

const std::string &FabIconUrl() { return GetAppUi().icon_url; }

std::string FabIconCachePath() {
    static std::string path;
    if (!path.empty()) return path;
    char buf[256]{};
    if (FILE *fp = std::fopen(OBF("/proc/self/cmdline"), "r")) {
        std::fread(buf, 1, sizeof(buf) - 1, fp);
        std::fclose(fp);
    }
    std::string pkg(buf);
    const auto colon = pkg.find(':');
    if (colon != std::string::npos) pkg.resize(colon);
    path = std::string(OBF("/data/user/0/")) + pkg + OBF("/files/fab.png");
    return path;
}

void DropLoadIconCache() {
    g_load_icon_tex = 0;
    g_load_icon_path[0] = '\0';
}

ImTextureID LoadIcon(const char *path) {
    if (!path) {
        if (g_load_icon_tex) glDeleteTextures(1, &g_load_icon_tex);
        DropLoadIconCache();
        return (ImTextureID) 0;
    }
    if (!path[0]) return (ImTextureID) 0;
    if (g_load_icon_tex && strncmp(g_load_icon_path, path, sizeof(g_load_icon_path)) == 0) {
        return (ImTextureID) (intptr_t) g_load_icon_tex;
    }

    int w = 0, h = 0, ch = 0;
    unsigned char *px = stbi_load(path, &w, &h, &ch, 4);
    if (!px || w <= 0 || h <= 0) {
        LOGE(OBF("LoadIcon stbi_load FAIL path=%s w=%d h=%d reason=%s"), path, w, h, stbi_failure_reason() ? stbi_failure_reason() : OBF("?"));
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
    return (ImTextureID) (intptr_t) g_load_icon_tex;
}

std::string MetaPath(const char *save_path) { return std::string(save_path) + OBFS(".etag"); }

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
    const std::string content = v.etag + OBFS("\n") + v.last_modified + OBFS("\n");
    const auto wr = fs::WriteBytes(meta_path, content.data(), content.size());
    if (!wr.ok()) LOGW(OBF("WriteMeta failed path=%s: %s"), meta_path.c_str(), wr.error.c_str());
}

bool DownloadIcon(const char *url, const char *save_path) {
    if (!url || !url[0] || !save_path || !save_path[0]) {
        LOGE(OBF("DownloadIcon invalid args url=%p path=%p"), (void *) url, (void *) save_path);
        return false;
    }
    const int64_t existing = fs::IsFile(save_path) ? fs::FileSize(save_path) : -1;
    if (existing == 0) {
        LOGW(OBF("DownloadIcon removing stale 0-byte file path=%s"), save_path);
        fs::Remove(save_path);
        fs::Remove(MetaPath(save_path));
    }
    const std::string dir = fs::Dirname(save_path);
    if (!dir.empty()) {
        const auto mk = fs::MkdirP(dir);
        if (!mk.error.empty()) LOGW(OBF("DownloadIcon MkdirP %s: %s"), dir.c_str(), mk.error.c_str());
    }
    const std::string meta_path = MetaPath(save_path);
    http::CacheValidators in;
    http::CacheValidators out;
    const bool have_cache = fs::IsFile(save_path) && fs::FileSize(save_path) > 0;
    const bool have_meta = ReadMeta(meta_path, &in);
    const http::CacheValidators *in_ptr = nullptr;
    if (have_cache && have_meta && (!in.etag.empty() || !in.last_modified.empty())) in_ptr = &in;
    const http::Response resp = http::DownloadConditional(url, save_path, 20, in_ptr, &out);
    const int64_t after = fs::IsFile(save_path) ? fs::FileSize(save_path) : -1;
    if (resp.cache_hit()) return after > 0;
    if (resp.ok()) {
        WriteMeta(meta_path, out);
        DropLoadIconCache();
        g_fab_icon = (ImTextureID) 0;
        return after > 0;
    }
    LOGW(OBF("DownloadIcon failed status=%ld %s"), resp.status, resp.error.empty() ? OBF("(none)") : resp.error.c_str());
    fs::Remove(save_path);
    fs::Remove(meta_path);
    return false;
}

void PollFabDownload() {
    if (g_fab_icon) return;
    if (g_fab_file_ready.load(std::memory_order_acquire)) return;
    const std::string &url = FabIconUrl();
    if (url.empty()) return;
    const std::string path = FabIconCachePath();
    if (g_fab_fail_count.load(std::memory_order_acquire) >= kMaxFabRetries) return;
    const int64_t now = NowMs();
    if (now < g_fab_next_retry_ms.load(std::memory_order_acquire)) return;
    bool expected = false;
    if (!g_fab_downloading.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) return;
    std::thread([path, url, now] {
        const bool ok = DownloadIcon(url.c_str(), path.c_str());
        if (ok) {
            g_fab_file_ready.store(true, std::memory_order_release);
        } else {
            const int n = g_fab_fail_count.fetch_add(1, std::memory_order_acq_rel) + 1;
            g_fab_next_retry_ms.store(now + kFabBackoffMs, std::memory_order_release);
            if (n >= kMaxFabRetries) LOGW(OBF("FabDownload abandoned after %d failures path=%s"), n, path.c_str());
        }
        g_fab_downloading.store(false, std::memory_order_release);
    }).detach();
}

} // namespace

void InvalidateIcon() {
    g_fab_icon = (ImTextureID) 0;
    DropLoadIconCache();
}

ImTextureID GetFabIcon() {
    if (!g_fab_icon) {
        PollFabDownload();
        if (g_fab_file_ready.load(std::memory_order_acquire)) {
            const std::string path = FabIconCachePath();
            g_fab_icon = LoadIcon(path.c_str());
            if (!g_fab_icon) {
                LOGW(OBF("GetFabIcon LoadIcon failed — removing %s and retry later"), path.c_str());
                fs::Remove(path);
                g_fab_file_ready.store(false, std::memory_order_release);
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
