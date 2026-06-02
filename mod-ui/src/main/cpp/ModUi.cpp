#include "ModUi.hpp"
#include "Internal.hpp"
#include "Theme.hpp"

#define LOGGER_TAG "ATTACK_ModUi"
#include <Includes/Logger.h>

#include <JNIHelper/JNIHelper.hpp>
#include <imgui.h>

namespace modui {

static AppUi g_app_ui;
static bool g_ctx = false;
static bool g_natives = false;
static bool g_visible = true;
static bool g_expanded = true;
static OverlayDrawFn g_overlay_draws[16]{};
static int g_overlay_count = 0;

bool RegisterOverlayDraw(OverlayDrawFn draw) {
    if (!draw || g_overlay_count >= 16) return false;
    for (int i = 0; i < g_overlay_count; ++i) {
        if (g_overlay_draws[i] == draw) return true;
    }
    g_overlay_draws[g_overlay_count++] = draw;
    return true;
}

void RunOverlayDraws() {
    for (int i = 0; i < g_overlay_count; ++i) {
        if (g_overlay_draws[i]) g_overlay_draws[i]();
    }
}

bool Init() {
    if (!g_ctx) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        io.IniFilename = nullptr;
        io.ConfigFlags |= ImGuiConfigFlags_IsTouchScreen;
        io.ConfigDragScroll = true;
        io.ConfigWindowsMoveFromTitleBarOnly = true;
        io.DragScrollButton = ImGuiMouseButton_Left;
        SetupTheme();
        SetupUiFonts();
        g_ctx = true;
        LOGI(OBF("context ready (%s)"), IMGUI_VERSION);
    }
    if (g_natives) return true;
    JNIEnv *env = jni::Env();
    if (!env || !RegisterSurfaceNatives(env)) {
        LOGE(OBF("RegisterSurfaceNatives failed"));
        return false;
    }
    g_natives = true;
    return true;
}

void Shutdown() {
    if (!g_ctx) return;
    ImGui::DestroyContext();
    g_ctx = false;
}

void SetAppUi(const AppUi &ui) { g_app_ui = ui; }

const AppUi &GetAppUi() { return g_app_ui; }

void SetMenuVisible(bool visible) { g_visible = visible; }

bool MenuVisible() { return g_visible; }

void SetMenuExpanded(bool expanded) { g_expanded = expanded; }

bool MenuExpanded() { return g_expanded; }

} // namespace modui
