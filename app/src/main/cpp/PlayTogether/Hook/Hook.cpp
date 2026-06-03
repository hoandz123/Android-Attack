#include "Hook.h"
#include "AntiCheat.h"
#include "AutoFishing/AutoFishing.h"
#include "AutoFishing/PickerSnapshot.h"
#include "../Config/Config.h"
#include "SDK/FrameWork.h"
#include "SDK/ActorControl.h"
#include "SDK/CombineContent.h"
#include "SDK/DialogBaseLock.h"
#include "SDK/NetWebProtocol.h"
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
    AutoFishing::InitPickerCache();
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
    Tools::Hook((void *)GET_METHOD("NetNativeProtocol", "SendToFishingCasting", 1), (void *)AutoFishing::onSendToFishingCasting, (void **)&AutoFishing::old_SendToFishingCasting);
    Tools::Hook((void *)GET_METHOD("NetNativeProtocol", "PID_Crafting_Start", 1), (void *)CombineContent::onPID_Crafting_Start, (void **)&CombineContent::old_PID_Crafting_Start);
    Tools::Hook((void *)GET_METHOD("NetNativeProtocol", "PID_Crafting_TimeDecrease", 1), (void *)CombineContent::onPID_Crafting_TimeDecrease, (void **)&CombineContent::old_PID_Crafting_TimeDecrease);
    Tools::Hook((void *)GET_METHOD("NetNativeProtocol", "PID_Crafting_ItemReward", 1), (void *)CombineContent::onPID_Crafting_ItemReward, (void **)&CombineContent::old_PID_Crafting_ItemReward);
    Tools::Hook((void *)GET_METHOD("NetNativeProtocol", "PID_ITEM_Combine", 1), (void *)CombineContent::onPID_ITEM_Combine, (void **)&CombineContent::old_PID_ITEM_Combine);
    Tools::Hook((void *)GET_METHOD("CombineContent", "ShowCraftingRewardDialog", 1), (void *)CombineContent::onShowCraftingRewardDialog, nullptr);
    DialogBaseLock::installHooks();
    Tools::Hook((void *)GET_METHOD("NetWebProtocol", "PID_SHOP_Buy", 1), (void *)NetWebProtocol::onPID_SHOP_Buy, (void **)&NetWebProtocol::old_PID_SHOP_Buy);
    Tools::Hook((void *)GET_METHOD("NetWebProtocol", "PID_SHOP_BuyList", 1), (void *)NetWebProtocol::onPID_SHOP_BuyList, (void **)&NetWebProtocol::old_PID_SHOP_BuyList);
    Tools::Hook((void *)GET_METHOD("ActorDefaultControlPlayer", "ShowFishingZoneTitle", 1), (void *)AutoFishing::onShowFishingZoneTitle, (void **)&AutoFishing::old_ShowFishingZoneTitle);
    LOGI(OBF("Hook init done"));
}

}
