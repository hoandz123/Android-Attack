#include <imgui.h>
#include <mod_ui.hpp>

namespace appui {

static void DrawAttackTab() {
    if (ImGui::CollapsingHeader("Attack", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::TextUnformatted("Nội dung plugin — thay toàn bộ vùng bên phải.");
        static bool demo = true;
        ImGui::Checkbox("Example toggle", &demo);
        static char buf[128] = "";
        ImGui::InputText("ImGui text", buf, sizeof(buf));
    }
}

static void DrawAboutTab() {
    ImGui::Text("mod-ui + :app");
    ImGui::Separator();
    ImGui::Text("ImGui %s", IMGUI_VERSION);
    ImGui::TextWrapped(
        "Đăng ký tab bằng modui::AppUi::add_tab(id, nhãn, hàm_vẽ). "
        "Shell giữ khung trái/phải; plugin chỉ vẽ panel content.");
}

void RegisterMenu() {
    modui::AppUi ui{};
    ui.set_window_title("Android Attack##modui_shell");
    ui.add_tab("attack", "Attack", DrawAttackTab);
    ui.add_tab("about", "About", DrawAboutTab);
    modui::SetAppUi(ui);
}

} // namespace appui
