#include "mod_ui.hpp"
#include "mod_ui_internal.hpp"
#include "mod_ui_theme.hpp"
#include <JNIHelper/JNIHelper.hpp>
#include <android/log.h>
#include <imgui.h>

#define TAG "ModUi"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

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
