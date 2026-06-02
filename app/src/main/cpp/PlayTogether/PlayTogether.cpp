#include "../Games.hpp"
#include "Hook/Hook.h"
#include "UI/MenuUi.h"
#include <API/Il2CppApi.h>
#define LOGGER_TAG "ATTACK_PlayTogether"
#include <Includes/Logger.h>
#include <ModUi.hpp>
#include <thread>

namespace playtogether {

void Activate() {
    SetupMenuUi();
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
