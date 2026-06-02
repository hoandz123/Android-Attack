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

const MenuLayoutConfig &GetMenuLayout();
void SetDisplayDensity(float density);
float GetDisplayDensity();
float DpToPx(float dp);
bool DisplayMetricsReady();
void ResetInitialLayout();
bool ApplyInitialLayout(const ImVec2 *size_px = nullptr);
void ApplyResizeConstraints();
ImVec2 MenuWindowSize();
ImVec2 MenuWindowPos(const ImVec2 &window_size);

} // namespace modui
