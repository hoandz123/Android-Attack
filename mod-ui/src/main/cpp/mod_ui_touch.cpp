#include "mod_ui_internal.hpp"

#include <algorithm>
#include <imgui.h>

namespace modui {

namespace {

float g_x, g_y;
bool g_down, g_dirty;
float g_inset_l, g_inset_t, g_inset_r, g_inset_b;

} // namespace

void FeedTouch(int action, float x, float y) {
    g_x = x;
    g_y = y;
    g_dirty = true;
    if (action == 0) g_down = true;
    else if (action == 1 || action == 3) g_down = false;
}

void SetSafeInsets(float left, float top, float right, float bottom) {
    g_inset_l = left;
    g_inset_t = top;
    g_inset_r = right;
    g_inset_b = bottom;
}

void ApplyPendingTouch() {
    if (!g_dirty && !g_down) return;
    ImGuiIO &io = ImGui::GetIO();
    io.AddMouseSourceEvent(ImGuiMouseSource_TouchScreen);
    io.AddMousePosEvent(g_x, g_y);
    io.AddMouseButtonEvent(0, g_down);
    g_dirty = false;
}

void ApplySafeAreaStyle() {
    ImGui::GetStyle().DisplaySafeAreaPadding =
        ImVec2(std::max(g_inset_l, g_inset_r), std::max(g_inset_t, g_inset_b));
}

} // namespace modui
