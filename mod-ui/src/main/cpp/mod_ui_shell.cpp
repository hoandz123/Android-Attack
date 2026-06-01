#include "mod_ui.hpp"

#include <imgui.h>

namespace modui {

namespace {

void apply_shell_style() {
    ImGuiStyle &s = ImGui::GetStyle();
    s.WindowRounding = 8.f;
    s.FrameRounding = 4.f;
    s.GrabRounding = 4.f;
    s.WindowBorderSize = 1.f;
    ImVec4 *c = s.Colors;
    c[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.09f, 0.12f, 0.94f);
    c[ImGuiCol_TitleBgActive] = ImVec4(0.18f, 0.35f, 0.58f, 1.f);
}

} // namespace

void draw_menu_shell(const AppUi &ui) {
    if (!menu_visible()) return;
    apply_shell_style();
    const ImGuiViewport *vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(420.f, 320.f), ImGuiCond_FirstUseEver);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;
    if (!ImGui::Begin("Mod Menu", nullptr, flags)) {
        ImGui::End();
        return;
    }
    if (ImGui::BeginTabBar("modui_tabs", ImGuiTabBarFlags_None)) {
        if (ImGui::BeginTabItem("Menu")) {
            ImGui::TextUnformatted("Plugin UI — implement in :app");
            ImGui::Separator();
            if (ui.draw_menu) ui.draw_menu();
            else ImGui::TextDisabled("Register modui::AppUi::draw_menu from app.");
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("About")) {
            ImGui::Text("mod-ui shell + render");
            ImGui::Text("ImGui %s", IMGUI_VERSION);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::End();
}

} // namespace modui
