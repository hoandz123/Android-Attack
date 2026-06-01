#include "mod_ui_internal.hpp"

#include <algorithm>
#include <imgui.h>
#include <mutex>

namespace modui {

namespace {

std::mutex g_mtx;
float g_x, g_y;
bool g_down, g_dirty;
float g_inset_l, g_inset_t, g_inset_r, g_inset_b;

} // namespace

void feed_touch(int action, float x, float y) {
    std::lock_guard lock(g_mtx);
    g_x = x;
    g_y = y;
    g_dirty = true;
    if (action == 0) g_down = true;
    else if (action == 1 || action == 3) g_down = false;
}

void set_safe_insets(float left, float top, float right, float bottom) {
    std::lock_guard lock(g_mtx);
    g_inset_l = left;
    g_inset_t = top;
    g_inset_r = right;
    g_inset_b = bottom;
}

void apply_pending_touch() {
    std::lock_guard lock(g_mtx);
    if (!g_dirty && !g_down) return;
    ImGuiIO &io = ImGui::GetIO();
    io.AddMouseSourceEvent(ImGuiMouseSource_TouchScreen);
    io.AddMousePosEvent(g_x, g_y);
    io.AddMouseButtonEvent(0, g_down);
    g_dirty = false;
}

void apply_safe_area_style() {
    float l, t, r, b;
    {
        std::lock_guard lock(g_mtx);
        l = g_inset_l;
        t = g_inset_t;
        r = g_inset_r;
        b = g_inset_b;
    }
    ImGui::GetStyle().DisplaySafeAreaPadding = ImVec2(std::max(l, r), std::max(t, b));
}

} // namespace modui
