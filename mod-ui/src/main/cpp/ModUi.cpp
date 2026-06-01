#include "ModUi.hpp"
#include "Internal.hpp"
#include "Theme.hpp"

#define LOG_TAG "ModUi"
#include <Includes/Logger.h>

#include <JNIHelper/JNIHelper.hpp>
#include <imgui.h>

namespace modui {

static AppUi g_app_ui;
static bool g_ctx = false;
static bool g_natives = false;
static bool g_visible = true;
static bool g_expanded = true;

bool Init() {
    if (!g_ctx) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        io.IniFilename = nullptr;
        io.ConfigFlags |= ImGuiConfigFlags_IsTouchScreen;
        SetupTheme();
        SetupUiFonts();
        g_ctx = true;
        LOGI("context ready (%s)", IMGUI_VERSION);
    }
    if (g_natives) return true;
    JNIEnv *env = jni::Env();
    if (!env || !RegisterSurfaceNatives(env)) {
        LOGE("RegisterSurfaceNatives failed");
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
