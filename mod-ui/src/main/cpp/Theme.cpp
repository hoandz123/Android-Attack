#include "Theme.hpp"

#include <imgui.h>

// Palette + metrics from Vk-Engine SetEditorStyleMoonlight()
// https://github.com/ostef/Vk-Engine/blob/main/Source/Editor/utils.jai
// (Moonlight style — deathsu / Madam-Herta)

namespace modui {

namespace {

constexpr float kUiScale = 3.0f;

inline void SetCol(ImVec4 *c, ImGuiCol idx, float r, float g, float b, float a) {
    c[idx] = ImVec4(r, g, b, a);
}

void ApplyVkEngineMoonlightMetrics(ImGuiStyle &s) {
    s.WindowMenuButtonPosition = ImGuiDir_Right;
    s.WindowTitleAlign = ImVec2(0.5f, 0.5f);
    s.WindowRounding = 4.f;
    s.ChildRounding = 3.f;
    s.FrameRounding = 3.f;
    s.PopupRounding = 2.f;
    s.ScrollbarRounding = 0.f;
    s.GrabRounding = 4.f;
    s.TabRounding = 3.f;
    s.WindowBorderSize = 1.f;
    s.ChildBorderSize = 1.f;
    s.PopupBorderSize = 1.f;
    s.FrameBorderSize = 1.f;
    s.SeparatorTextBorderSize = 1.f;
    s.FramePadding = ImVec2(6.f, 6.f);
    s.ItemSpacing = ImVec2(4.f, 4.f);
    s.ItemInnerSpacing = ImVec2(8.f, 6.f);
    s.CellPadding = ImVec2(6.f, 4.f);
    s.IndentSpacing = 20.f;
    s.ScrollbarSize = 20.f;
    s.GrabMinSize = 14.f;
}

void ApplyVkEngineMoonlightPalette(ImGuiStyle &s) {
    ImVec4 *c = s.Colors;

    SetCol(c, ImGuiCol_Text, 1.f, 1.f, 1.f, 1.f);
    SetCol(c, ImGuiCol_TextDisabled, 0.274f, 0.317f, 0.450f, 1.f);
    SetCol(c, ImGuiCol_WindowBg, 0.078f, 0.086f, 0.101f, 1.f);
    SetCol(c, ImGuiCol_ChildBg, 0.092f, 0.100f, 0.115f, 1.f);
    SetCol(c, ImGuiCol_PopupBg, 0.078f, 0.086f, 0.101f, 1.f);
    SetCol(c, ImGuiCol_Border, 0.156f, 0.168f, 0.192f, 0.250f);
    SetCol(c, ImGuiCol_BorderShadow, 0.078f, 0.086f, 0.101f, 0.250f);
    SetCol(c, ImGuiCol_FrameBg, 0.112f, 0.126f, 0.154f, 1.f);
    SetCol(c, ImGuiCol_FrameBgHovered, 0.156f, 0.168f, 0.192f, 1.f);
    SetCol(c, ImGuiCol_FrameBgActive, 0.156f, 0.168f, 0.192f, 1.f);
    SetCol(c, ImGuiCol_TitleBg, 0.047f, 0.054f, 0.070f, 1.f);
    SetCol(c, ImGuiCol_TitleBgActive, 0.047f, 0.054f, 0.070f, 1.f);
    SetCol(c, ImGuiCol_TitleBgCollapsed, 0.078f, 0.086f, 0.101f, 1.f);
    SetCol(c, ImGuiCol_MenuBarBg, 0.098f, 0.105f, 0.121f, 1.f);
    SetCol(c, ImGuiCol_ScrollbarBg, 0.047f, 0.054f, 0.070f, 1.f);
    SetCol(c, ImGuiCol_ScrollbarGrab, 0.117f, 0.133f, 0.149f, 1.f);
    SetCol(c, ImGuiCol_ScrollbarGrabHovered, 0.156f, 0.168f, 0.192f, 1.f);
    SetCol(c, ImGuiCol_ScrollbarGrabActive, 0.117f, 0.133f, 0.149f, 1.f);
    SetCol(c, ImGuiCol_CheckMark, 0.972f, 1.f, 0.498f, 1.f);
    SetCol(c, ImGuiCol_SliderGrab, 0.971f, 1.f, 0.498f, 1.f);
    SetCol(c, ImGuiCol_SliderGrabActive, 1.f, 0.795f, 0.498f, 1.f);
    SetCol(c, ImGuiCol_Button, 0.117f, 0.133f, 0.149f, 1.f);
    SetCol(c, ImGuiCol_ButtonHovered, 0.182f, 0.189f, 0.197f, 1.f);
    SetCol(c, ImGuiCol_ButtonActive, 0.154f, 0.154f, 0.154f, 1.f);
    SetCol(c, ImGuiCol_Header, 0.049f, 0.482f, 0.863f, 1.f);
    SetCol(c, ImGuiCol_HeaderHovered, 0.038f, 0.321f, 0.715f, 1.f);
    SetCol(c, ImGuiCol_HeaderActive, 0.123f, 0.551f, 0.928f, 1.f);
    SetCol(c, ImGuiCol_Separator, 0.275f, 0.318f, 0.451f, 0.78f);
    SetCol(c, ImGuiCol_SeparatorHovered, 0.275f, 0.318f, 0.451f, 1.f);
    SetCol(c, ImGuiCol_SeparatorActive, 1.f, 1.f, 1.f, 1.f);
    SetCol(c, ImGuiCol_ResizeGrip, 0.145f, 0.145f, 0.145f, 1.f);
    SetCol(c, ImGuiCol_ResizeGripHovered, 0.972f, 1.f, 0.498f, 1.f);
    SetCol(c, ImGuiCol_ResizeGripActive, 0.999f, 1.f, 0.999f, 1.f);
    SetCol(c, ImGuiCol_Tab, 0.f, 0.f, 0.f, 0.f);
    SetCol(c, ImGuiCol_TabHovered, 0.04f, 0.32f, 0.71f, 0.78f);
    SetCol(c, ImGuiCol_TabSelected, 0.05f, 0.48f, 0.86f, 0.78f);
    SetCol(c, ImGuiCol_TabSelectedOverline, 0.05f, 0.48f, 0.86f, 1.f);
    SetCol(c, ImGuiCol_TabDimmed, 0.f, 0.f, 0.f, 0.f);
    SetCol(c, ImGuiCol_TabDimmedSelected, 0.11f, 0.13f, 0.16f, 0.39f);
    SetCol(c, ImGuiCol_TabDimmedSelectedOverline, 0.11f, 0.13f, 0.16f, 0.78f);
    SetCol(c, ImGuiCol_PlotLines, 0.521f, 0.600f, 0.701f, 1.f);
    SetCol(c, ImGuiCol_PlotLinesHovered, 0.039f, 0.980f, 0.980f, 1.f);
    SetCol(c, ImGuiCol_PlotHistogram, 0.884f, 0.794f, 0.561f, 1.f);
    SetCol(c, ImGuiCol_PlotHistogramHovered, 0.957f, 0.957f, 0.957f, 1.f);
    SetCol(c, ImGuiCol_TableHeaderBg, 0.089f, 0.100f, 0.122f, 1.f);
    SetCol(c, ImGuiCol_TableBorderStrong, 0.047f, 0.054f, 0.070f, 1.f);
    SetCol(c, ImGuiCol_TableBorderLight, 0.141f, 0.161f, 0.208f, 0.2f);
    SetCol(c, ImGuiCol_TableRowBg, 0.117f, 0.133f, 0.149f, 1.f);
    SetCol(c, ImGuiCol_TableRowBgAlt, 0.098f, 0.105f, 0.121f, 1.f);
    SetCol(c, ImGuiCol_TextLink, 0.049f, 0.482f, 0.863f, 1.f);
    SetCol(c, ImGuiCol_TextSelectedBg, 0.935f, 0.935f, 0.935f, 0.376f);
    SetCol(c, ImGuiCol_DragDropTarget, 0.498f, 0.513f, 1.f, 1.f);
    SetCol(c, ImGuiCol_NavCursor, 0.266f, 0.289f, 1.f, 1.f);
    SetCol(c, ImGuiCol_NavWindowingHighlight, 0.498f, 0.513f, 1.f, 1.f);
    SetCol(c, ImGuiCol_NavWindowingDimBg, 0.196f, 0.176f, 0.545f, 0.501f);
    SetCol(c, ImGuiCol_ModalWindowDimBg, 0.239f, 0.233f, 0.342f, 0.501f);
}

} // namespace

float UiScale() { return kUiScale; }

void SetupTheme() {
    ImGuiStyle &s = ImGui::GetStyle();
    s = ImGuiStyle();
    ApplyVkEngineMoonlightMetrics(s);
    ApplyVkEngineMoonlightPalette(s);
    s.ScaleAllSizes(kUiScale);
}

} // namespace modui
