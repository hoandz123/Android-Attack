#include "EspHelper.h"
#include "EspRuntime.h"
#include "../Config/Config.h"
#include <DrawRender.hpp>
#include <GLES3/gl3.h>
#include <GameUI/EspGUI.h>
#include <cstdint>
#include <imgui.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace lienquan {
namespace EspHelper {

namespace Draw {
namespace {

constexpr float kMinMapIconSz = 6.f;

struct GlTex {
    GLuint id = 0;
    uint32_t version = 0;
};

struct ViewScale {
    float x = 1.f;
    float y = 1.f;
    float size = 1.f;
};

std::unordered_map<std::string, GlTex> gTex;
std::unordered_map<unsigned int, std::string> gObjKey;

ImTextureID GlTexId(GLuint t) { return t ? (ImTextureID) (uint64_t) t : (ImTextureID) 0; }

ViewScale CalcViewScale(const EspRuntime::Snapshot &s) {
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    ViewScale scale{};
    if (s.screenW > 1.f && s.screenH > 1.f && display.x > 1.f && display.y > 1.f) {
        scale.x = display.x / s.screenW;
        scale.y = display.y / s.screenH;
        scale.size = (scale.x + scale.y) * .5f;
    }
    return scale;
}

ImVec2 ScalePos(float x, float y, const ViewScale &scale) { return {x * scale.x, y * scale.y}; }

ImU32 ToCol32(const float c[4]) {
    return IM_COL32((int) (c[0] * 255.f + .5f), (int) (c[1] * 255.f + .5f), (int) (c[2] * 255.f + .5f), (int) (c[3] * 255.f + .5f));
}

GLuint UploadGl(const std::vector<uint8_t> &px, int w, int h) {
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

ImTextureID TextureForKey(const char *key) {
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

void DrawMapIcon(ImDrawList *dl, ImTextureID tex, float cx, float cy, float sz) {
    if (sz < kMinMapIconSz) sz = kMinMapIconSz;
    const float r = sz * .5f;
    const ImVec2 p0(cx - r, cy - r), p1(cx + r, cy + r);
    const ImU32 shadow = ToCol32(gLQConfig.esp.iconShadowColor);
    const ImU32 border = ToCol32(gLQConfig.esp.iconBorderColor);
    dl->AddCircleFilled({cx, cy}, r + 1.5f, shadow, 24);
    if (tex) {
        dl->AddImageRounded(tex, p0, p1, ImVec2(0, 0), ImVec2(1, 1), IM_COL32_WHITE, r, ImDrawFlags_RoundCornersAll);
    } else {
        dl->AddCircleFilled({cx, cy}, r, shadow, 24);
    }
    dl->AddCircle({cx, cy}, r, border, 24, 1.4f);
}

void UpdateObjKey(const EspRuntime::Snapshot &s) {
    for (int i = 0; i < s.iconCount && i < EspRuntime::kMaxHeroIcons; ++i) {
        const EspRuntime::IconItem &icon = s.icons[i];
        if (!icon.valid || !icon.key[0] || !icon.objId) continue;
        gObjKey[icon.objId] = icon.key;
    }
}

void DrawIconsOnMap(ImDrawList *dl, const EspRuntime::Snapshot &s) {
    const ViewScale scale = CalcViewScale(s);
    for (int i = 0; i < s.targetCount && i < EspRuntime::kMaxTargets; ++i) {
        const EspRuntime::MapItem &item = s.mapItems[i];
        if (!item.valid || !item.objId) continue;
        const char *key = item.key[0] ? item.key : nullptr;
        if (!key) {
            auto ki = gObjKey.find(item.objId);
            if (ki == gObjKey.end()) continue;
            key = ki->second.c_str();
        }
        const ImVec2 pos = ScalePos(item.x, item.y, scale);
        const float cfgR = gLQConfig.esp.miniMapIconSize > 0.f ? gLQConfig.esp.miniMapIconSize : 30.f;
        DrawMapIcon(dl, TextureForKey(key), pos.x, pos.y, cfgR * 2.f * scale.size);
    }
}

void DrawHeroIcons(ImDrawList *dl, const EspRuntime::Snapshot &s) {
    if (!dl || !gLQConfig.esp.showHeroIcons || !s.valid) return;
    UpdateObjKey(s);
    if (s.targetCount > 0) DrawIconsOnMap(dl, s);
}

void DrawLine(ImDrawList *dl, const EspRuntime::LineItem &line, float thick) {
    if (!line.valid) return;
    dl->AddLine({line.fromX, line.fromY}, {line.toX, line.toY}, ToCol32(gLQConfig.esp.lineColor), thick);
}

} // namespace

void Overlay() {
    if (!gLQConfig.esp.enabled && !gLQConfig.esp.showHeroIcons) return;
    EspRuntime::Snapshot snap{};
    if (!EspRuntime::ReadSnapshot(snap)) return;
    ImDrawList *dl = ImGui::GetBackgroundDrawList();
    if (!dl) return;
    DrawHeroIcons(dl, snap);
    if (!gLQConfig.esp.enabled || !snap.hasMyWorld || snap.targetCount <= 0) return;
    const float thick = gLQConfig.esp.lineThickness > 0.f ? gLQConfig.esp.lineThickness : EspGUI::kDefaultLineThickness;
    for (int i = 0; i < snap.targetCount && i < EspRuntime::kMaxTargets; ++i)
        DrawLine(dl, snap.lines[i], thick);
}

void Register() {
    static bool once = false;
    if (once) return;
    DrawRender::registerTask(Overlay);
    once = true;
}

} // namespace Draw

}
}
