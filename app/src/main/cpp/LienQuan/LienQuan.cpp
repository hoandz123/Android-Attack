#include "../Games.hpp"
#include "Hook/Hook.h"
#include "UI/Tab/Chinh.h"
#include "UI/Tab/Esp.h"
#include <API/Il2CppApi.h>
#define LOGGER_TAG "ATTACK_LienQuan"
#include <Includes/Logger.h>
#include <Includes/obfuscate.h>
#include <ModUi.hpp>
#include <string>
#include <thread>

namespace lienquan {

void Activate() {
    modui::AppUi ui{};
    ui.menu_size = ImVec2(480.f, 340.f);
    ui.icon_url = OBF("https://tools-mod.com/storage/brand/logo.png");
    ui.set_window_title(OBF("Liên Quân##modui_shell"));
    ui.add_tab(OBF("main"), OBF("Chính"), DrawChinhPage);
    ui.add_tab(OBF("esp"), OBF("ESP"), DrawEspPage);
    modui::SetAppUi(ui);
    RegisterEspOverlay();

    std::thread([]() {
        Init_Il2cpp_Symbol();
        if (!il2cpp_loaded.load()) {
            LOGE(OBF("[LienQuan] il2cpp chua load"));
            return;
        }
        LOGI(OBF("[LienQuan] il2cpp da load, bat dau dump + hook"));
        const std::string dumpPath = Il2CppDomain::dump_domain();
        if (dumpPath.empty()) {
            LOGW(OBF("[LienQuan] dump_domain that bai"));
        } else {
            LOGI(OBF("[LienQuan] dump_domain -> %s"), dumpPath.c_str());
        }
        Hook::init();
    }).detach();
}

REGISTER_GAME(OBF("com.garena.game.kgvn"), Activate);

} // namespace lienquan
