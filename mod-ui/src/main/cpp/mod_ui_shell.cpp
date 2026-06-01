#include "mod_ui.hpp"
#include "mod_ui_internal.hpp"
#include "mod_ui_layout.hpp"

#include <imgui.h>

namespace modui {

void draw_menu_shell(const AppUi &ui) {
    if (!menu_visible()) return;
    const ImVec2 size = menu_window_size();
    ImGui::SetNextWindowPos(menu_window_pos(size), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(size, ImGuiCond_FirstUseEver);
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
