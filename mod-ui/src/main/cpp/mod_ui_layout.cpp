#include "mod_ui_layout.hpp"

#include <algorithm>
#include <imgui.h>

namespace modui {

namespace {

MenuLayoutConfig g_menu_layout;
float g_density = 1.f;

} // namespace

const MenuLayoutConfig &menu_layout_config() { return g_menu_layout; }

void set_display_density(float density) {
    if (density > 0.1f && density < 10.f) g_density = density;
}

float display_density() { return g_density; }

float dp_to_px(float dp) { return dp * g_density; }

ImVec2 menu_window_size() {
    const MenuLayoutConfig &cfg = g_menu_layout;
    const ImGuiViewport *vp = ImGui::GetMainViewport();
    const ImVec2 work = vp->WorkSize;

    float w = dp_to_px(cfg.width_dp);
    float h = dp_to_px(cfg.height_dp);
    const float min_w = dp_to_px(cfg.min_width_dp);
    const float min_h = dp_to_px(cfg.min_height_dp);
    const float max_w = work.x * cfg.max_width_screen;
    const float max_h = work.y * cfg.max_height_screen;

    w = std::clamp(w, min_w, max_w);
    h = std::clamp(h, min_h, max_h);
    return ImVec2(w, h);
}

ImVec2 menu_window_pos(const ImVec2 &window_size) {
    const MenuLayoutConfig &cfg = g_menu_layout;
    const ImGuiViewport *vp = ImGui::GetMainViewport();
    const float margin = dp_to_px(cfg.margin_dp);
    const ImVec2 work_pos = vp->WorkPos;
    const ImVec2 work = vp->WorkSize;

    const float x = work_pos.x + std::max(margin, (work.x - window_size.x) * 0.5f);
    const float y = work_pos.y + std::max(margin, (work.y - window_size.y) * 0.5f);
    return ImVec2(x, y);
}

} // namespace modui
