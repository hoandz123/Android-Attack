#include "../Games.hpp"
#include <API/Il2CppApi.h>
#include <imgui.h>
#include <Includes/obfuscate.h>
#include <ModUi.hpp>
#include <Tools/Tools.h>
#include <thread>

#define LOG_TAG OBF("AttackPlugin")
#include <Includes/Logger.h>

namespace playtogether {

static void DrawMainTab() {
    if (ImGui::CollapsingHeader(OBF("Play Together"), ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::TextUnformatted(OBF("Menu Play Together — thêm tab/feature tại đây."));
        static bool speedDemo = false;
        ImGui::Checkbox(OBF("Speed hack (demo)"), &speedDemo);
        static bool flyDemo = false;
        ImGui::Checkbox(OBF("Fly (demo)"), &flyDemo);
    }
}

void Activate() {
    modui::AppUi ui{};
    ui.set_window_title(OBF("Play Together##modui_shell"));
    ui.add_tab(OBF("main"), OBF("Chính"), DrawMainTab);
    modui::SetAppUi(ui);
    std::thread([]() {
        Init_Il2cpp_Symbol();
        if (!il2cpp_loaded.load()) {
            LOGE(OBF("[PlayTogether] il2cpp chua load"));
            return;
        }
        LOGI(OBF("[PlayTogether] il2cpp da load, bat dau hook"));
    }).detach();
}

REGISTER_GAME(OBF("com.vng.playtogether"), Activate);

} // namespace playtogether
