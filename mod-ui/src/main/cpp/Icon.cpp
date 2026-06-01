#include "Icon.hpp"

#include <FileManager.hpp>
#include <HttpClient.hpp>
#include <GLES3/gl3.h>
#include <cstring>

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
    if (!path[0]) return (ImTextureID) 0;
    if (tex && strncmp(cached, path, sizeof(cached)) == 0) return (ImTextureID) (intptr_t) tex;

    int w = 0, h = 0, ch = 0;
    unsigned char *px = stbi_load(path, &w, &h, &ch, 4);
    if (!px) return (ImTextureID) 0;

    if (tex) glDeleteTextures(1, &tex);
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
    stbi_image_free(px);

    strncpy(cached, path, sizeof(cached) - 1);
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

} // namespace modui
