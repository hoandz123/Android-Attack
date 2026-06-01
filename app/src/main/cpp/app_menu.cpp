#include <imgui.h>
#include <mod_ui.hpp>

namespace appui {

static void draw_plugin_menu() {
    if (ImGui::CollapsingHeader("Attack", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::TextUnformatted("Custom widgets live in :app");
        static bool demo = true;
        ImGui::Checkbox("Example toggle", &demo);
        static char buf[128] = "";
        ImGui::InputText("ImGui text", buf, sizeof(buf));
    }
}

void register_menu() {
    modui::AppUi ui{};
    ui.draw_menu = draw_plugin_menu;
    modui::set_app_ui(ui);
}

} // namespace appui
