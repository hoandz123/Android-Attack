#include "../Games.hpp"
#include "HOOK_PLAY/HOOK_PLAY.h"
#include "UI/MenuUi.h"
#include "PlayLog.h"
#include <API/Il2CppApi.h>
#include <Includes/obfuscate.h>
#include <ModUi.hpp>
#include <thread>
#include <atomic>

namespace playtogether {

namespace {

std::atomic<bool> s_activateOnce{false};

}

void Activate() {
    bool expected = false;
    if (!s_activateOnce.compare_exchange_strong(expected, true)) {
        return;
    }
    SetupMenuUi();
    std::thread([]() {
        Init_Il2cpp_Symbol();
        if (!il2cpp_loaded.load()) {
            LOGE(OBF("[PlayTogether] il2cpp chua load"));
            return;
        }
        LOGI(OBF("[PlayTogether] il2cpp da load, bat dau hook"));
        HOOK_PLAY::init();
    }).detach();
}

REGISTER_GAME(OBF("com.vng.playtogether"), Activate);

}
