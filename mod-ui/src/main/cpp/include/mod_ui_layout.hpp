#pragma once

struct ImVec2;

namespace modui {

struct MenuLayoutConfig {
    /** Hình chữ nhật ngang (rộng > cao), dp — chỉ dùng cho lần mở đầu. */
    float width_dp = 624.f;
    float height_dp = 442.f;
    float min_width_dp = 364.f;
    float min_height_dp = 286.f;
    float margin_dp = 12.f;
    float max_width_screen = 0.92f;
    float max_height_screen = 0.88f;
};

const MenuLayoutConfig &menu_layout_config();
void set_display_density(float density);
float display_density();
float dp_to_px(float dp);
bool display_metrics_ready();
void reset_menu_initial_layout();
bool try_apply_initial_menu_layout();
void menu_apply_resize_constraints();
ImVec2 menu_window_size();
ImVec2 menu_window_pos(const ImVec2 &window_size);

} // namespace modui
