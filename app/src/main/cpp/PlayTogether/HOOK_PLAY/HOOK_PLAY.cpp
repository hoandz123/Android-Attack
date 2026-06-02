#include "HOOK_PLAY.h"
#include "AntiCheat.h"
#include "../Config/Config.h"
#include <DrawRender.hpp>
#include <GameUI/EspGUI.h>
#include "../UI/InfoWindow.h"
#include "../UI/OverlaySnapshot.h"
#include "../SDK/FrameWork.h"
#include "../SDK/ActorControl.h"
#include <API/Il2CppApi.h>
#include <Includes/obfuscate.h>

#define LOG_TAG OBF("ATTACK_PlayTogether")

#include <Includes/Logger.h>
#include <Tools/Tools.h>
#include <imgui.h>
#include <thread>
#include <atomic>

namespace HOOK_PLAY {

namespace {

std::atomic<bool> s_initOnce{false};

}

void DRAW_RENDER() {
    ShowInfoWindow();
    if (gPLConfig.fishing.enabled && gPLConfig.fishing.showStatus && !gPLConfig.general.isInfo) {
        OverlaySnapshot::View snap{};
        OverlaySnapshot::Read(snap);
        if (snap.ready) {
            char buf[96];
            snprintf(buf, sizeof(buf), OBF("Câu: %s | #%d"), OverlaySnapshot::FishingStateLabel(snap.fishingState), snap.fishCaught);
            EspGUI::DrawTooltip(ImVec2(12.f, 48.f), buf);
        }
    }
}

void init() {
    bool expected = false;
    if (!s_initOnce.compare_exchange_strong(expected, true)) {
        LOGI(OBF("HOOK_PLAY init skipped (already done)"));
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
    Tools::Hook(ActorControl::get_class()->find_method(OBF("get_Kunit"), 0)->methodPointer, (void *) ActorControl::get_Kunit, (void **) &ActorControl::old_get_Kunit);
    DrawRender::registerTask(DRAW_RENDER);
    LOGI(OBF("HOOK_PLAY init done"));
}

}
