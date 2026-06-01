#pragma once

struct ImVec2;

namespace modui {

struct MenuLayoutConfig {
    float width_dp = 320.f;
    float height_dp = 400.f;
    float min_width_dp = 260.f;
    float min_height_dp = 220.f;
    float margin_dp = 12.f;
    float max_width_screen = 0.92f;
    float max_height_screen = 0.88f;
};

const MenuLayoutConfig &menu_layout_config();
void set_display_density(float density);
float display_density();
float dp_to_px(float dp);
ImVec2 menu_window_size();
ImVec2 menu_window_pos(const ImVec2 &window_size);

} // namespace modui
