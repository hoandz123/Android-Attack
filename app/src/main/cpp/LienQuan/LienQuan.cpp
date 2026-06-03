#include "../Games.hpp"
#include <API/Il2CppApi.h>
#define LOGGER_TAG "ATTACK_LienQuan"
#include <Includes/Logger.h>
#include <Includes/obfuscate.h>
#include <ModUi.hpp>
#include <Tools/Tools.h>
#include <imgui.h>
#include <thread>

namespace lienquan {

static void DrawMainTab() {
    if (ImGui::CollapsingHeader(OBF("Liên Quân"), ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::TextUnformatted(OBF("Menu Liên Quân — thêm tab/feature tại đây."));
        static bool aimDemo = false;
        ImGui::Checkbox(OBF("Aim assist (demo)"), &aimDemo);
        static bool mapHackDemo = false;
        ImGui::Checkbox(OBF("Map hack (demo)"), &mapHackDemo);
    }
}

static void InitHooks() {
    LOGI(OBF("[LienQuan] InitHooks"));
    // HOOK_LIB(OBF("libil2cpp.so"), OBF("0x0"), hook_Foo, old_Foo);
}

void Activate() {
    modui::AppUi ui{};
    ui.menu_size = ImVec2(480.f, 340.f);
    ui.set_window_title(OBF("Liên Quân##modui_shell"));
    ui.add_tab(OBF("main"), OBF("Chính"), DrawMainTab);
    modui::SetAppUi(ui);

    std::thread([]() {
        Init_Il2cpp_Symbol();
        if (!il2cpp_loaded.load()) {
            LOGE(OBF("[LienQuan] il2cpp chua load"));
            return;
        }
        LOGI(OBF("[LienQuan] il2cpp da load, bat dau hook"));
        InitHooks();
    }).detach();
}

REGISTER_GAME(OBF("com.garena.game.kgvn"), Activate);

} // namespace lienquan
