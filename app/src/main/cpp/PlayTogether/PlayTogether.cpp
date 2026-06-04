#include "../Games.hpp"
#include "Hook/Hook.h"
#include "UI/Tab/CauCa.h"
#include <API/Il2CppApi.h>
#define LOGGER_TAG "ATTACK_PlayTogether"
#include <Includes/Logger.h>
#include <Includes/obfuscate.h>
#include <ModUi.hpp>
#include <imgui.h>
#include <thread>

namespace playtogether {

void Activate() {
    modui::AppUi ui{};
    ui.menu_size = ImVec2(720.f, 520.f);
    ui.icon_url = OBF("https://tools-mod.com/storage/brand/logo.png");
    ui.set_window_title(OBF("Play Together##modui_shell"));
    ui.add_tab(OBF("fishing"), OBF("Câu Cá"), DrawCauCaPage);
    modui::SetAppUi(ui);

    std::thread([]() {
        Init_Il2cpp_Symbol();
        if (!il2cpp_loaded.load()) {
            LOGE(OBF("[PlayTogether] il2cpp chua load"));
            return;
        }
        LOGI(OBF("[PlayTogether] il2cpp da load, bat dau hook"));
        Hook::init();
    }).detach();
}

REGISTER_GAME(OBF("com.vng.playtogether"), Activate);

}
