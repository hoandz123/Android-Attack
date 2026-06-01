#include "mod_ui.hpp"

#include <android/log.h>
#include <imgui.h>
#include <imgui_impl_android.h>
#include <imgui_impl_opengl3.h>

#define TAG "ModUi"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

namespace modui {

void draw_menu_shell(const AppUi &ui);

static bool g_backend = false;

bool set_surface(ANativeWindow *window) {
    if (!init()) return false;
    if (g_backend) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplAndroid_Shutdown();
        g_backend = false;
    }
    if (!window) return true;
    if (!ImGui_ImplAndroid_Init(window)) {
        LOGE("ImGui_ImplAndroid_Init failed");
        return false;
    }
    if (!ImGui_ImplOpenGL3_Init("#version 300 es")) {
        LOGE("ImGui_ImplOpenGL3_Init failed");
        ImGui_ImplAndroid_Shutdown();
        return false;
    }
    g_backend = true;
    return true;
}

bool has_surface() { return g_backend; }

void begin_frame() {
    if (!g_backend) return;
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplAndroid_NewFrame();
    ImGui::NewFrame();
    draw_menu_shell(app_ui());
}

void end_frame() {
    if (!g_backend) return;
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

} // namespace modui
