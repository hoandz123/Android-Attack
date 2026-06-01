#include "Internal.hpp"
#include "Theme.hpp"

#define LOG_TAG OBF("ModUi")
#include <Includes/Logger.h>

#include <imgui.h>
#include "FontData.h"

namespace modui {

namespace {

constexpr float kBaseFontPx = 13.0f;

} // namespace

void SetupUiFonts() {
    ImGuiIO &io = ImGui::GetIO();
    io.Fonts->Clear();

    const ImWchar *ranges = io.Fonts->GetGlyphRangesVietnamese();

    ImFontConfig cfg;
    cfg.OversampleH = 2;
    cfg.OversampleV = 2;
    cfg.PixelSnapH = true;
    cfg.FontDataOwnedByAtlas = true;

    const float font_px = kBaseFontPx * UiScale();
    ImFont *font = io.Fonts->AddFontFromMemoryTTF(
        const_cast<unsigned char *>(kModUiFontData),
        static_cast<int>(kModUiFontSize),
        font_px,
        &cfg,
        ranges);
    if (!font) {
        LOGE(OBF("FreeSans load failed, using default font"));
        io.Fonts->AddFontDefault();
    }
}

} // namespace modui
