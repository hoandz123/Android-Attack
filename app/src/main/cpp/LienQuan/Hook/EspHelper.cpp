#include "EspHelper.h"
#include "EspRuntime.h"
#include "../Config/Config.h"
#include <DrawRender.hpp>
#include <GLES3/gl3.h>
#include <GameUI/EspGUI.h>
#include <cmath>
#include <cstdint>
#include <cstdio>
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

ImU32 HpBarFillColor(int hp, int maxHp, bool dead) {
    if (dead) return IM_COL32(90, 90, 90, 140);
    if (maxHp <= 0) return IM_COL32(120, 120, 120, 180);
    const float ratio = static_cast<float>(hp) / static_cast<float>(maxHp);
    if (ratio <= 0.25f) return IM_COL32(255, 60, 60, 220);
    if (ratio <= 0.5f) return IM_COL32(255, 200, 50, 220);
    return IM_COL32(60, 200, 80, 220);
}

void DrawHpBar(ImDrawList *dl, float x, float y, float barW, float barH, int hp, int maxHp, bool dead) {
    if (!dl || barW < 4.f || barH < 2.f || maxHp <= 0) return;
    float ratio = static_cast<float>(hp) / static_cast<float>(maxHp);
    if (ratio < 0.f) ratio = 0.f;
    if (ratio > 1.f) ratio = 1.f;
    const ImVec2 p0(x, y);
    const ImVec2 p1(x + barW, y + barH);
    dl->AddRectFilled(p0, p1, IM_COL32(20, 20, 20, 170), 2.f);
    if (ratio > 0.f) {
        const ImVec2 f1(x + barW * ratio, y + barH);
        dl->AddRectFilled(p0, f1, HpBarFillColor(hp, maxHp, dead), 2.f);
    }
    dl->AddRect(p0, p1, IM_COL32(0, 0, 0, 200), 2.f);
}

void DrawCooldownSlots(ImDrawList *dl, float firstCx, float cy, const EspRuntime::InfoItem &item, float scaleSize) {
    if (!dl || !item.hasCooldowns) return;
    const float dotR = (gLQConfig.esp.cooldownDotSize > 0.f ? gLQConfig.esp.cooldownDotSize : 5.f) * scaleSize * 0.5f;
    const float spacing = (gLQConfig.esp.cooldownSpacing > 0.f ? gLQConfig.esp.cooldownSpacing : 16.f) * scaleSize;
    const float fontSize = (gLQConfig.esp.cooldownTextSize > 0.f ? gLQConfig.esp.cooldownTextSize : 11.f) * scaleSize;
    ImFont *font = ImGui::GetFont();
    const ImU32 readyCol = IM_COL32(80, 220, 120, 245);
    const ImU32 readyGlow = IM_COL32(140, 255, 170, 120);
    const ImU32 cdCol = IM_COL32(255, 230, 90, 240);
    const ImU32 lockedCol = IM_COL32(95, 95, 95, 170);
    const ImU32 shadow = IM_COL32(0, 0, 0, 170);
    float cx = firstCx;
    for (int i = 0; i < LActorRoot::kSkillCooldownSlots; ++i) {
        const LActorRoot::SkillCooldownSlot &slot = item.cooldownSlots[i];
        if (!slot.valid || !slot.unlocked) {
            dl->AddCircleFilled({cx, cy}, dotR, lockedCol, 16);
        } else if (slot.ready) {
            dl->AddCircleFilled({cx, cy}, dotR + 1.2f, readyGlow, 16);
            dl->AddCircleFilled({cx, cy}, dotR, readyCol, 16);
        } else {
            char buf[8]{};
            const int showSec = slot.cdSec > 999 ? 999 : (slot.cdSec > 0 ? slot.cdSec : 0);
            snprintf(buf, sizeof(buf), "%d", showSec);
            const ImVec2 textSize = font ? font->CalcTextSizeA(fontSize, FLT_MAX, 0.f, buf) : ImGui::CalcTextSize(buf);
            const float tx = cx - textSize.x * 0.5f;
            const float ty = cy - textSize.y * 0.5f;
            dl->AddText(font, fontSize, {tx + 1.f, ty + 1.f}, shadow, buf);
            dl->AddText(font, fontSize, {tx, ty}, cdCol, buf);
        }
        cx += spacing;
    }
}

void DrawOffscreenArrow(ImDrawList *dl, float cx, float cy, float angle, float size, ImU32 col) {
    if (!dl || size < 4.f) return;
    const float cosA = std::cosf(angle);
    const float sinA = std::sinf(angle);
    const ImVec2 tip(cx + cosA * size, cy + sinA * size);
    const ImVec2 left(cx + std::cosf(angle + 2.4f) * size * 0.65f, cy + std::sinf(angle + 2.4f) * size * 0.65f);
    const ImVec2 right(cx + std::cosf(angle - 2.4f) * size * 0.65f, cy + std::sinf(angle - 2.4f) * size * 0.65f);
    dl->AddTriangleFilled(tip, left, right, col);
    dl->AddTriangle(tip, left, right, IM_COL32(0, 0, 0, 200), 1.f);
}

void DrawInfoText(ImDrawList *dl, const EspRuntime::Snapshot &s) {
    if (!dl || !s.valid) return;
    const bool showText = gLQConfig.esp.showInfo;
    const bool showHp = gLQConfig.esp.showHpBar;
    const bool showCd = gLQConfig.esp.showCooldowns;
    if (!showText && !showHp && !showCd) return;
    const ViewScale scale = CalcViewScale(s);
    const ImU32 baseCol = ToCol32(gLQConfig.esp.infoColor);
    const ImU32 shadow = IM_COL32(0, 0, 0, 180);
    const ImU32 lowHpCol = IM_COL32(255, 110, 50, 245);
    const float fontSize = 14.f * scale.size;
    const float barW = (gLQConfig.esp.hpBarWidth > 0.f ? gLQConfig.esp.hpBarWidth : 80.f) * scale.size;
    const float barH = (gLQConfig.esp.hpBarHeight > 0.f ? gLQConfig.esp.hpBarHeight : 8.f) * scale.size;
    const float barOffX = gLQConfig.esp.hpBarOffsetX * scale.x;
    const float barOffY = gLQConfig.esp.hpBarOffsetY * scale.y;
    const bool horizontal = gLQConfig.esp.infoLayout == 1;
    for (int i = 0; i < s.targetCount && i < EspRuntime::kMaxTargets; ++i) {
        const EspRuntime::InfoItem &item = s.infoItems[i];
        if (!item.valid) continue;
        const bool hasText = showText && item.text[0];
        const bool hasHpDraw = showHp && item.hasHp && item.maxHp > 0;
        const bool hasCdDraw = showCd && item.hasCooldowns;
        if (!hasText && !hasHpDraw && !hasCdDraw) continue;
        const ImVec2 pos = ScalePos(item.x, item.y, scale);
        ImVec2 textSize(0.f, 0.f);
        if (hasText) {
            const bool lowHp = gLQConfig.esp.lowHpHighlight && item.lowHp;
            const ImU32 col = lowHp ? lowHpCol : baseCol;
            const char *drawText = item.text;
            char lowBuf[96]{};
            if (lowHp) {
                snprintf(lowBuf, sizeof(lowBuf), "! %s", item.text);
                drawText = lowBuf;
            }
            dl->AddText(nullptr, fontSize, {pos.x + 1.f, pos.y + 1.f}, shadow, drawText);
            dl->AddText(nullptr, fontSize, pos, col, drawText);
            textSize = ImGui::CalcTextSize(drawText);
        }
        float barX = pos.x + barOffX;
        float barY = pos.y + (hasText ? textSize.y + barOffY : barOffY);
        if (horizontal && hasText) {
            barX = pos.x + textSize.x + 4.f + barOffX;
            barY = pos.y + barOffY;
        }
        if (hasHpDraw) {
            DrawHpBar(dl, barX, barY, barW, barH, item.hp, item.maxHp, item.isDead);
        }
        if (hasCdDraw) {
            const float spacing = (gLQConfig.esp.cooldownSpacing > 0.f ? gLQConfig.esp.cooldownSpacing : 16.f) * scale.size;
            const float dotD = (gLQConfig.esp.cooldownDotSize > 0.f ? gLQConfig.esp.cooldownDotSize : 5.f) * scale.size;
            float cdY = barY;
            if (hasHpDraw) cdY += barH;
            cdY += gLQConfig.esp.cooldownOffsetY * scale.y;
            const float cdX = (horizontal && hasText ? barX : pos.x + barOffX) + gLQConfig.esp.cooldownOffsetX * scale.x;
            const float firstCx = cdX + spacing * 0.5f;
            const float cdCenterY = cdY + dotD * 0.5f;
            DrawCooldownSlots(dl, firstCx, cdCenterY, item, scale.size);
        }
    }
}

void DrawOffscreenArrows(ImDrawList *dl, const EspRuntime::Snapshot &s) {
    if (!dl || !gLQConfig.esp.offscreenArrow || !s.valid) return;
    const ViewScale scale = CalcViewScale(s);
    const ImU32 col = ToCol32(gLQConfig.esp.infoColor);
    const float size = (gLQConfig.esp.arrowSize > 0.f ? gLQConfig.esp.arrowSize : 14.f) * scale.size;
    for (int i = 0; i < s.targetCount && i < EspRuntime::kMaxTargets; ++i) {
        const EspRuntime::ArrowItem &arrow = s.arrows[i];
        if (!arrow.valid) continue;
        const ImVec2 pos = ScalePos(arrow.x, arrow.y, scale);
        DrawOffscreenArrow(dl, pos.x, pos.y, arrow.angle, size, col);
    }
}

} // namespace

void Overlay() {
    if (!gLQConfig.esp.enabled && !gLQConfig.esp.showHeroIcons && !gLQConfig.esp.showInfo && !gLQConfig.esp.showHpBar && !gLQConfig.esp.showCooldowns && !gLQConfig.esp.offscreenArrow) return;
    EspRuntime::Snapshot snap{};
    if (!EspRuntime::ReadSnapshot(snap)) return;
    ImDrawList *dl = ImGui::GetBackgroundDrawList();
    if (!dl) return;
    DrawHeroIcons(dl, snap);
    DrawOffscreenArrows(dl, snap);
    DrawInfoText(dl, snap);
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
