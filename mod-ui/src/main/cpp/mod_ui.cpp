#include "mod_ui.hpp"

#include <android/log.h>
#include <imgui.h>

#define TAG "ModUi"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)

namespace modui {

static AppUi g_app_ui;
static bool g_ctx = false;
static bool g_visible = true;

bool init() {
    if (g_ctx) return true;
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.IniFilename = nullptr;
    ImGui::StyleColorsDark();
    g_ctx = true;
    LOGI("context ready (%s)", IMGUI_VERSION);
    return true;
}

void shutdown() {
    if (!g_ctx) return;
    ImGui::DestroyContext();
    g_ctx = false;
}

void set_app_ui(const AppUi &ui) { g_app_ui = ui; }

const AppUi &app_ui() { return g_app_ui; }

void set_menu_visible(bool visible) { g_visible = visible; }

bool menu_visible() { return g_visible; }

} // namespace modui
