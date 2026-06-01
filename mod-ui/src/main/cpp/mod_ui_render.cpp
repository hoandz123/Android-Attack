#include "mod_ui.hpp"
#include "mod_ui_internal.hpp"
#include "mod_ui_layout.hpp"
#include <GLES3/gl3.h>
#include <android/log.h>
#include <imgui.h>
#include <imgui_impl_android.h>
#include <imgui_impl_opengl3.h>

#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "ModUi", __VA_ARGS__)

namespace modui {

void draw_menu_shell(const AppUi &ui);

static bool g_backend;

bool set_surface(ANativeWindow *window) {
    if (!init()) return false;
    if (g_backend) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplAndroid_Shutdown();
        g_backend = false;
        reset_menu_initial_layout();
    }
    if (!window) return true;
    if (!ImGui_ImplAndroid_Init(window)
        || !ImGui_ImplOpenGL3_Init("#version 300 es")) {
        LOGE("imgui backend init failed");
        ImGui_ImplAndroid_Shutdown();
        return false;
    }
    g_backend = true;
    return true;
}

bool has_surface() { return g_backend; }

void begin_frame() {
    if (!g_backend) return;
    apply_safe_area_style();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplAndroid_NewFrame();
    apply_pending_touch();
    ImGui::NewFrame();
    draw_menu_shell(app_ui());
    apply_pending_keyboard();
}

void end_frame() {
    if (!g_backend) return;
    ImGui::Render();
    ImDrawData *draw = ImGui::GetDrawData();
    if (!draw) return;
    const int w = (int) (draw->DisplaySize.x * draw->FramebufferScale.x);
    const int h = (int) (draw->DisplaySize.y * draw->FramebufferScale.y);
    if (w > 0 && h > 0) {
        glViewport(0, 0, w, h);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_SCISSOR_TEST);
        glClearColor(0.f, 0.f, 0.f, 0.f);
        glClear(GL_COLOR_BUFFER_BIT);
    }
    ImGui_ImplOpenGL3_RenderDrawData(draw);
    sync_soft_keyboard(ImGui::GetIO().WantTextInput);
}

} // namespace modui
