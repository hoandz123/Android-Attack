#include "Hook.h"
#include "AntiCheat.h"
#include "AutoFishing/AutoFishing.h"
#include "../Config/Config.h"
#include "SDK/FrameWork.h"
#include "SDK/ActorControl.h"
#include <API/Il2CppApi.h>
#define LOGGER_TAG "ATTACK_PlayTogether"
#include <Includes/Logger.h>
#include <Tools/Tools.h>
#include <thread>
#include <atomic>

namespace Hook {

namespace {

std::atomic<bool> s_initOnce{false};

}

void init() {
    bool expected = false;
    if (!s_initOnce.compare_exchange_strong(expected, true)) {
        LOGI(OBF("Hook init skipped (already done)"));
        return;
    }
    LoadConfig();
    AntiCheat::init();
    std::thread([]() {
        Tools::Sleep(5);
        while (true) {
            Tools::Sleep(1);
            if (Object *obj = FrameWork::get_Instance()) {
                Object *AntiCheatListener = obj->get_field_object<Object *>("AntiCheatListener");
                if (AntiCheatListener) {
                    LOGI(OBF("AntiCheatListener found, disabling..."));
                    obj->set_field_object("AntiCheatListener", nullptr);
                    LOGI(OBF("AntiCheatListener disabled"));
                    break;
                }
            }
        }
    }).detach();
    Tools::Hook((void *)GET_METHOD("ActorControl", "get_Kunit", 0), (void *)ActorControl::get_Kunit, (void **)&ActorControl::old_get_Kunit);
    Tools::Hook((void *)GET_METHOD("FishingSystem", "ReceiveFishingCatch", 1), (void *)AutoFishing::onReceiveFishingCatch, (void **)&AutoFishing::old_ReceiveFishingCatch);
    Tools::Hook((void *)GET_METHOD("NetNativeProtocol", "PID_FISHING_CASTING", 1), (void *)AutoFishing::onPID_FISHING_CASTING, (void **)&AutoFishing::old_PID_FISHING_CASTING);
    LOGI(OBF("Hook init done"));
}

}
