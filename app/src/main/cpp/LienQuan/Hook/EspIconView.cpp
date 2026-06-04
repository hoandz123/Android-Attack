#include "EspIconView.h"
#include "../Config/Config.h"
#include <GLES3/gl3.h>
#include <Includes/obfuscate.h>
#include <string>
#include <unordered_map>
#include <vector>

#define LOGGER_TAG "ATTACK_EspIcon"
#include <Includes/Logger.h>

namespace lienquan::EspIconView {
namespace {

constexpr float kMapSz = 28.f;

struct GlTex {
    GLuint id = 0;
    uint32_t version = 0;
};

std::unordered_map<std::string, GlTex> gTex;
std::unordered_map<unsigned int, std::string> gObjKey;

static ImTextureID GlTexId(GLuint t) { return t ? (ImTextureID) (uint64_t) t : (ImTextureID) 0; }

static GLuint UploadGl(const std::vector<uint8_t> &px, int w, int h) {
    if (px.size() < (size_t) w * h * 4) return 0;
    GLuint t = 0;
    glGenTextures(1, &t);
    if (!t) return 0;
    glBindTexture(GL_TEXTURE_2D, t);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    return t;
}

static ImTextureID TextureForKey(const char *key) {
    if (!key || !key[0]) return (ImTextureID) 0;
    std::vector<uint8_t> rgba;
    int w = 0, h = 0;
    uint32_t version = 0;
    if (!EspRuntime::GetIconPixels(key, rgba, w, h, version)) {
        auto it = gTex.find(key);
        return it != gTex.end() ? GlTexId(it->second.id) : (ImTextureID) 0;
    }
    auto &tex = gTex[key];
    if (tex.id && tex.version == version) return GlTexId(tex.id);
    if (tex.id) glDeleteTextures(1, &tex.id);
    tex.id = UploadGl(rgba, w, h);
    tex.version = version;
    return GlTexId(tex.id);
}

static void DrawCell(ImDrawList *dl, ImTextureID tex, float cx, float cy, float sz) {
    const ImVec2 p0(cx - sz * .5f, cy - sz * .5f), p1(cx + sz * .5f, cy + sz * .5f);
    if (tex) {
        dl->AddImage(tex, p0, p1);
        dl->AddRect(p0, p1, IM_COL32(255, 255, 255, 160));
    } else {
        dl->AddRectFilled(p0, p1, IM_COL32(40, 40, 40, 180));
        dl->AddRect(p0, p1, IM_COL32(120, 120, 120, 200));
    }
}

static void DrawStrip(ImDrawList *dl, const EspRuntime::Snapshot &s) {
    if (s.iconCount <= 0) return;
    dl->AddText({12.f, s.icons[0].y - s.icons[0].size * .5f - 18.f},
                IM_COL32(200, 220, 255, 255), OBF("Hero icons"));
    for (int i = 0; i < s.iconCount && i < EspRuntime::kMaxStripIcons; ++i) {
        const EspRuntime::IconItem &icon = s.icons[i];
        if (!icon.valid || !icon.key[0]) continue;
        if (icon.objId) gObjKey[icon.objId] = icon.key;
        DrawCell(dl, TextureForKey(icon.key), icon.x, icon.y, icon.size);
    }
}

static void DrawOnMap(ImDrawList *dl, const EspRuntime::Snapshot &s, float dotR) {
    for (int i = 0; i < s.targetCount && i < EspRuntime::kMaxTargets; ++i) {
        const EspRuntime::MapItem &item = s.mapItems[i];
        if (!item.valid || !item.objId) continue;
        auto ki = gObjKey.find(item.objId);
        if (ki == gObjKey.end()) continue;
        DrawCell(dl, TextureForKey(ki->second.c_str()), item.x, item.y - dotR - kMapSz * .55f, kMapSz);
    }
}

} // namespace

void Draw(ImDrawList *dl, const EspRuntime::Snapshot &snapshot) {
    if (!dl || !gLQConfig.esp.showHeroIcons || !snapshot.valid) return;
    DrawStrip(dl, snapshot);
    if (gLQConfig.esp.minimapDot && snapshot.targetCount > 0) {
        const float r = gLQConfig.esp.minimapDotRadius > 0.f ? gLQConfig.esp.minimapDotRadius : 3.f;
        DrawOnMap(dl, snapshot, r);
    }
}

} // namespace lienquan::EspIconView
