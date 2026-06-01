#include "mod_ui_icon.hpp"

#include <FileManager.hpp>
#include <HttpClient.hpp>
#include <GLES3/gl3.h>
#include <cstring>
#include <string>
#include <vector>

#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace modui {

ImTextureID LoadIcon(const char *path) {
    static char cached[8192]{};
    static GLuint tex = 0;

    if (!path) {
        if (tex) glDeleteTextures(1, &tex);
        tex = 0;
        cached[0] = '\0';
        return (ImTextureID) 0;
    }

    if (tex && strncmp(cached, path, sizeof(cached)) == 0) return (ImTextureID) (intptr_t) tex;

    if (tex) glDeleteTextures(1, &tex);
    strncpy(cached, path, sizeof(cached) - 1);

    int w = 0, h = 0;
    unsigned char *px = nullptr;
    if (strncmp(path, "http", 4) == 0) {
        const http::Response r = http::Get(path, 20);
        px = stbi_load_from_memory(r.body.data(), (int) r.body.size(), &w, &h, nullptr, 4);
    } else {
        px = stbi_load(path, &w, &h, nullptr, 4);
    }
    if (!px) return (ImTextureID) 0;

    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
    stbi_image_free(px);

    return (ImTextureID) (intptr_t) tex;
}

bool DownloadIcon(const char *url, const char *save_path) {
    if (!url || !save_path) return false;

    const std::string meta = std::string(save_path) + ".url";
    bool cache_ok = fs::IsFile(save_path) && fs::IsFile(meta);
    if (cache_ok) {
        fs::Result r;
        const std::vector<uint8_t> b = fs::ReadBytes(meta, &r);
        const size_t n = std::strlen(url);
        cache_ok = r.ok() && b.size() == n && std::memcmp(b.data(), url, n) == 0;
    }

    if (!cache_ok) {
        fs::MkdirP(fs::Dirname(save_path));
        if (!http::Download(url, save_path, 20).ok()) return false;
        fs::WriteBytes(meta, url, std::strlen(url));
    }

    return fs::IsFile(save_path);
}

} // namespace modui
