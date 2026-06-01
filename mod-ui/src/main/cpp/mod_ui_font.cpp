#include "mod_ui_internal.hpp"
#include "mod_ui_theme.hpp"

#include <android/log.h>
#include <imgui.h>

#include "mod_ui_font_data.h"

#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "ModUi", __VA_ARGS__)

namespace modui {

namespace {

constexpr float kBaseFontPx = 13.0f;

} // namespace

void setup_ui_fonts() {
    ImGuiIO &io = ImGui::GetIO();
    io.Fonts->Clear();

    const ImWchar *ranges = io.Fonts->GetGlyphRangesVietnamese();

    ImFontConfig cfg;
    cfg.OversampleH = 2;
    cfg.OversampleV = 2;
    cfg.PixelSnapH = true;
    cfg.FontDataOwnedByAtlas = true;

    const float font_px = kBaseFontPx * ui_scale();
    ImFont *font = io.Fonts->AddFontFromMemoryTTF(
        const_cast<unsigned char *>(kModUiFontData),
        static_cast<int>(kModUiFontSize),
        font_px,
        &cfg,
        ranges);
    if (!font) {
        LOGE("FreeSans load failed, using default font");
        io.Fonts->AddFontDefault();
    }
}

} // namespace modui
