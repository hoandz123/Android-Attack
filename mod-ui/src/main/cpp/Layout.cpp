#include "Layout.hpp"

#include <algorithm>
#include <cfloat>
#include <imgui.h>

namespace modui {

namespace {

MenuLayoutConfig g_menu_layout;
float g_density = 1.f;
bool g_metrics_ready = false;
bool g_initial_layout_done = false;

} // namespace

const MenuLayoutConfig &GetMenuLayout() { return g_menu_layout; }

void SetDisplayDensity(float density) {
    if (density > 0.1f && density < 10.f) {
        g_density = density;
        g_metrics_ready = true;
    }
}

float GetDisplayDensity() { return g_density; }

bool DisplayMetricsReady() { return g_metrics_ready; }

void ResetInitialLayout() { g_initial_layout_done = false; }

bool ApplyInitialLayout(const ImVec2 *size_px) {
    if (!g_metrics_ready || g_initial_layout_done) return false;
    const ImVec2 size = size_px ? *size_px : MenuWindowSize();
    ImGui::SetNextWindowPos(MenuWindowPos(size), ImGuiCond_Always);
    ImGui::SetNextWindowSize(size, ImGuiCond_Always);
    g_initial_layout_done = true;
    return true;
}

void ApplyResizeConstraints() {
    const MenuLayoutConfig &cfg = g_menu_layout;
    const ImVec2 min_sz(DpToPx(cfg.min_width_dp), DpToPx(cfg.min_height_dp));
    ImGui::SetNextWindowSizeConstraints(min_sz, ImVec2(FLT_MAX, FLT_MAX));
}

float DpToPx(float dp) { return dp * g_density; }

ImVec2 MenuWindowSize() {
    const MenuLayoutConfig &cfg = g_menu_layout;
    const ImGuiViewport *vp = ImGui::GetMainViewport();
    const ImVec2 work = vp->WorkSize;

    float w = DpToPx(cfg.width_dp);
    float h = DpToPx(cfg.height_dp);
    const float min_w = DpToPx(cfg.min_width_dp);
    const float min_h = DpToPx(cfg.min_height_dp);
    const float max_w = work.x * cfg.max_width_screen;
    const float max_h = work.y * cfg.max_height_screen;

    w = std::clamp(w, min_w, max_w);
    h = std::clamp(h, min_h, max_h);
    return ImVec2(w, h);
}

ImVec2 MenuWindowPos(const ImVec2 &window_size) {
    const MenuLayoutConfig &cfg = g_menu_layout;
    const ImGuiViewport *vp = ImGui::GetMainViewport();
    const float margin = DpToPx(cfg.margin_dp);
    const ImVec2 work_pos = vp->WorkPos;
    const ImVec2 work = vp->WorkSize;

    const float x = work_pos.x + std::max(margin, (work.x - window_size.x) * 0.5f);
    const float y = work_pos.y + std::max(margin, (work.y - window_size.y) * 0.5f);
    return ImVec2(x, y);
}

} // namespace modui
