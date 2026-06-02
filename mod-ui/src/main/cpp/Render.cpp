#include "ModUi.hpp"
#include "Internal.hpp"
#include "Layout.hpp"
#include "Icon.hpp"

#define LOG_TAG OBF("ModUi")
#include <Includes/Logger.h>

#include <GLES3/gl3.h>
#include <imgui.h>
#include <imgui_impl_android.h>
#include <imgui_impl_opengl3.h>

namespace modui {

void DrawMenuShell(const AppUi &ui);

static bool g_backend;

bool SetSurface(ANativeWindow *window) {
    if (!Init()) return false;
    if (g_backend) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplAndroid_Shutdown();
        g_backend = false;
        ResetInitialLayout();
    }
    if (!window) return true;
    if (!ImGui_ImplAndroid_Init(window)
        || !ImGui_ImplOpenGL3_Init(OBF("#version 300 es"))) {
        LOGE(OBF("imgui backend init failed"));
        ImGui_ImplAndroid_Shutdown();
        return false;
    }
    InvalidateIcon();
    g_backend = true;
    return true;
}

bool HasSurface() { return g_backend; }

void BeginFrame() {
    if (!g_backend) return;
    ApplySafeAreaStyle();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplAndroid_NewFrame();
    ApplyPendingTouch();
    ImGui::NewFrame();
    RunOverlayDraws();
    DrawMenuShell(GetAppUi());
    ApplyPendingKeyboard();
}

void EndFrame() {
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
    SyncSoftKeyboard(ImGui::GetIO().WantTextInput);
}

} // namespace modui
