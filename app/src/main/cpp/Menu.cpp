#include <imgui.h>
#include <Includes/obfuscate.h>
#include <ModUi.hpp>

namespace appui {

static void DrawAttackTab() {
    if (ImGui::CollapsingHeader(OBF("Attack"), ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::TextUnformatted(OBF("Nội dung plugin — thay toàn bộ vùng bên phải."));
        static bool demo = true;
        ImGui::Checkbox(OBF("Example toggle"), &demo);
        static char buf[128] = "";
        ImGui::InputText(OBF("ImGui text"), buf, sizeof(buf));
    }
}

static void DrawAboutTab() {
    ImGui::TextUnformatted(OBF("mod-ui + :app"));
    ImGui::Separator();
    ImGui::Text("%s %s", OBF("ImGui"), IMGUI_VERSION);
    ImGui::TextUnformatted(OBF("Đăng ký tab bằng modui::AppUi::add_tab(id, nhãn, hàm_vẽ). Shell giữ khung trái/phải; plugin chỉ vẽ panel content."));
}

void RegisterMenu() {
    modui::AppUi ui{};
    ui.set_window_title(OBF("Android Attack##modui_shell"));
    ui.add_tab(OBF("attack"), OBF("Attack"), DrawAttackTab);
    ui.add_tab(OBF("about"), OBF("About"), DrawAboutTab);
    modui::SetAppUi(ui);
}

} // namespace appui
