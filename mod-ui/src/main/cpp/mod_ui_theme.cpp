#include "mod_ui_theme.hpp"

#include <imgui.h>

namespace modui {

namespace {

constexpr float kUiScale = 3.0f;

void apply_palette(ImGuiStyle &s) {
    ImVec4 *c = s.Colors;
    c[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.09f, 0.12f, 0.94f);
    c[ImGuiCol_TitleBg] = ImVec4(0.12f, 0.14f, 0.18f, 1.f);
    c[ImGuiCol_TitleBgActive] = ImVec4(0.18f, 0.35f, 0.58f, 1.f);
    c[ImGuiCol_TitleBgCollapsed] = ImVec4(0.10f, 0.12f, 0.16f, 0.75f);
    c[ImGuiCol_Tab] = ImVec4(0.14f, 0.16f, 0.22f, 1.f);
    c[ImGuiCol_TabHovered] = ImVec4(0.22f, 0.40f, 0.62f, 1.f);
    c[ImGuiCol_TabActive] = ImVec4(0.18f, 0.35f, 0.58f, 1.f);
    c[ImGuiCol_FrameBg] = ImVec4(0.14f, 0.16f, 0.20f, 1.f);
    c[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.24f, 0.30f, 1.f);
    c[ImGuiCol_FrameBgActive] = ImVec4(0.24f, 0.30f, 0.38f, 1.f);
    c[ImGuiCol_Button] = ImVec4(0.20f, 0.38f, 0.58f, 1.f);
    c[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.46f, 0.68f, 1.f);
    c[ImGuiCol_ButtonActive] = ImVec4(0.16f, 0.32f, 0.50f, 1.f);
    c[ImGuiCol_Header] = ImVec4(0.20f, 0.38f, 0.58f, 0.55f);
    c[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.46f, 0.68f, 0.80f);
    c[ImGuiCol_HeaderActive] = ImVec4(0.18f, 0.35f, 0.58f, 1.f);
    c[ImGuiCol_CheckMark] = ImVec4(0.55f, 0.78f, 1.f, 1.f);
    c[ImGuiCol_SliderGrab] = ImVec4(0.35f, 0.58f, 0.86f, 1.f);
    c[ImGuiCol_SliderGrabActive] = ImVec4(0.45f, 0.68f, 0.95f, 1.f);
    c[ImGuiCol_Separator] = ImVec4(0.28f, 0.32f, 0.40f, 0.50f);
    c[ImGuiCol_Text] = ImVec4(0.92f, 0.94f, 0.98f, 1.f);
    c[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.54f, 0.62f, 1.f);
}

void apply_metrics(ImGuiStyle &s) {
    s.WindowRounding = 8.f;
    s.ChildRounding = 6.f;
    s.FrameRounding = 4.f;
    s.PopupRounding = 4.f;
    s.ScrollbarRounding = 4.f;
    s.GrabRounding = 4.f;
    s.TabRounding = 4.f;
    s.WindowBorderSize = 1.f;
    s.FrameBorderSize = 0.f;
    s.PopupBorderSize = 1.f;
}

} // namespace

float ui_scale() { return kUiScale; }

void setup_ui_theme() {
    ImGuiStyle &s = ImGui::GetStyle();
    ImGui::StyleColorsDark();
    apply_metrics(s);
    apply_palette(s);
    s.ScaleAllSizes(kUiScale);
}

} // namespace modui
