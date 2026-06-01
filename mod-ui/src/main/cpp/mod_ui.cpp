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

bool init() {
    if (!g_ctx) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        io.IniFilename = nullptr;
        io.ConfigFlags |= ImGuiConfigFlags_IsTouchScreen;
        setup_ui_theme();
        setup_ui_fonts();
        g_ctx = true;
        LOGI("context ready (%s)", IMGUI_VERSION);
    }
    if (g_natives) return true;
    JNIEnv *env = jni::env();
    if (!env || !register_surface_natives(env)) {
        LOGE("register_surface_natives failed");
        return false;
    }
    g_natives = true;
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

void set_menu_expanded(bool expanded) { g_expanded = expanded; }

bool menu_expanded() { return g_expanded; }

} // namespace modui
