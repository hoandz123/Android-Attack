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

void DrawFloatMarker(const OverlaySnapshot::View &snap) {
    if (!gPLConfig.fishing.showFloatMarker || !snap.hasFloatPoint) return;
    ImDrawList *dl = ImGui::GetForegroundDrawList();
    if (!dl) return;
    ImVec2 center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.55f);
    float scale = 28.f;
    float maxDist = 25.f;
    float dist = snap.floatDistance;
    if (dist > maxDist) dist = maxDist;
    float factor = dist / maxDist;
    float ox = snap.floatOffsetX * scale * 0.08f;
    float oz = snap.floatOffsetZ * scale * 0.08f;
    ImVec2 tip(center.x + ox, center.y - oz);
    ImU32 col = IM_COL32(80, 200, 255, 200);
    dl->AddLine(center, tip, col, 2.f);
    dl->AddCircleFilled(tip, 5.f + (1.f - factor) * 3.f, IM_COL32(120, 220, 255, 230));
    char distBuf[32];
    snprintf(distBuf, sizeof(distBuf), OBF("%.1fm"), snap.floatDistance);
    dl->AddText(ImVec2(tip.x + 8.f, tip.y - 8.f), IM_COL32(220, 240, 255, 255), distBuf);
}

}

void DRAW_RENDER() {
    ShowInfoWindow();
    if (!gPLConfig.fishing.enabled || !gPLConfig.fishing.showStatus) return;
    OverlaySnapshot::View snap{};
    OverlaySnapshot::Read(snap);
    if (!snap.ready) return;
    if (!gPLConfig.general.isInfo) {
        char buf[128];
        snprintf(buf, sizeof(buf), OBF("Câu: %s | #%d"), OverlaySnapshot::FishingStateLabel(snap.fishingState), snap.fishCaught);
        if (snap.pausedByRare) snprintf(buf, sizeof(buf), OBF("Câu: TẠM DỪNG (hiếm) | #%d"), snap.fishCaught);
        EspGUI::DrawTooltip(ImVec2(12.f, 48.f), buf);
    }
    DrawFloatMarker(snap);
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
