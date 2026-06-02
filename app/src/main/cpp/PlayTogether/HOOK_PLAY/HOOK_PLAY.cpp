#include "HOOK_PLAY.h"
#include "AntiCheat.h"
#include "../Config/Config.h"
#include "../Stubs/ESPManager.h"
#include <DrawRender.hpp>
#include <GameUI/EspGUI.h>
#include <GameUI/GameViewport.h>
#include "../RollCongCu.h"
#include "../UI/InfoWindow.h"
#include "../SDK/ActorDefaultControlPlayer.h"
#include "../SDK/NetNativeProtocol.h"
#include "../SDK/FrameWork.h"
#include "../SDK/ActorControl.h"
#include "../SDK/KinematicCharacterMotor.h"
#include "../SDK/DialogUnit.h"
#include "../SDK/DialogFishingGetItem.h"
#include "../SDK/EquipmentSystem.h"
#include "../SDK/MapGuideArrow.h"
#include "../SDK/HeadUpSelectButton.h"
#include "../SDK/DialogShopInGame.h"
#include "../SDK/TableFishingDifficultyImpl.h"
#include "../SDK/FishingSystem.h"
#include <API/Il2CppApi.h>
#include <API/Vector2.h>
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
    int glWidth = GameViewport::width();
    int glHeight = GameViewport::height();
    ESPManager::TickStale();
    const auto &espEntries = ESPManager::GetEntries();
    for (const auto &pair : espEntries) {
        const ESPEntry &e = pair.second;
        float scaleX = (float) ImGui::GetIO().DisplaySize.x / (float) glWidth;
        float scaleY = (float) ImGui::GetIO().DisplaySize.y / (float) glHeight;
        ImVec2 vitri = ImVec2(e.screenPos.x * scaleX, ImGui::GetIO().DisplaySize.y - (e.screenPos.y * scaleY));
        if (e.screenPos.z > 0) {
            ImVec2 lineStart = e.startPos == Vector2() ? ImVec2(ImGui::GetIO().DisplaySize.x / 2.f, 15.f) : ImVec2(e.startPos.x, e.startPos.y);
            EspGUI::DrawLine(lineStart, vitri, 2.f, e.color);
            if (!e.name.empty()) EspGUI::DrawTooltip(vitri, e.name.c_str());
            if (e.pos != Vector3()) {
                EspGUI::CircleBtn(vitri, Vector3(e.pos.x, e.pos.y + 5.f, e.pos.z), 25.f, [](Vector3 pos) { KinematicCharacterMotor::set_TransientPosition(pos); }, OBF("Tele"), true, IM_COL32(70, 70, 70, 255));
            }
        }
    }
    FishingSystem::DrawOverlay();
    ShowInfoWindow();
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
    Tools::Hook(DialogUnit::get_class()->find_method(OBF("DialogShow"), 0)->methodPointer, (void *) DialogUnit::DialogShow, (void **) &DialogUnit::old_DialogShow);
    Tools::Hook(DialogFishingGetItem::get_class()->find_method(OBF("OnItemSell"), 1)->methodPointer, (void *) DialogFishingGetItem::OnItemSell, (void **) &DialogFishingGetItem::old_OnItemSell);
    Tools::Hook(EquipmentSystem::get_class()->find_method(OBF("ShowRepairItem"), 2)->methodPointer, (void *) EquipmentSystem::ShowRepairItem, (void **) &EquipmentSystem::old_ShowRepairItem);
    Tools::Hook(ActorControl::get_class()->find_method(OBF("get_Kunit"), 0)->methodPointer, (void *) ActorControl::get_Kunit, (void **) &ActorControl::old_get_Kunit);
    if (NetNativeProtocol::get_class()) {
        Tools::Hook(NetNativeProtocol::get_class()->find_method(OBF("SendToFishingCasting"), 1)->methodPointer, (void *) ActorDefaultControlPlayer::Hook_SendToFishingCasting, (void **) &ActorDefaultControlPlayer::old_SendToFishingCasting);
    }
    if (Class *zoneTitle = FindClass(OBF("DialogZoneTitle"))) {
        Tools::Hook(zoneTitle->find_method(OBF("SetTitle"), 1)->methodPointer, (void *) ActorDefaultControlPlayer::Hook_DialogZoneTitle_SetTitle, (void **) &ActorDefaultControlPlayer::old_DialogZoneTitle_SetTitle);
    }
    Tools::Hook(KinematicCharacterMotor::get_class()->find_method(OBF("UpdatePhase1"), 1)->methodPointer, (void *) KinematicCharacterMotor::UpdatePhase1, (void **) &KinematicCharacterMotor::old_UpdatePhase1);
    Tools::Hook(MapGuideArrow::get_class()->find_method(OBF("Update"), 0)->methodPointer, (void *) MapGuideArrow::Update, (void **) &MapGuideArrow::old_Update);
    Tools::Hook(HeadUpSelectButton::get_class()->find_method(OBF("SetSprite"), 1)->methodPointer, (void *) HeadUpSelectButton::SetSprite, (void **) &HeadUpSelectButton::old_SetSprite);
    Tools::Hook(HeadUpSelectButton::get_class()->find_method(OBF("UpdatePosition"), 0)->methodPointer, (void *) HeadUpSelectButton::UpdatePosition, (void **) &HeadUpSelectButton::old_UpdatePosition);
    Tools::Hook(DialogShopInGame::get_class()->find_method(OBF("Update"), 0)->methodPointer, (void *) DialogShopInGame::Update, (void **) &DialogShopInGame::old_Update);
    TableFishingDifficultyImpl::init();
    ItemAffixOptionView_NS::init();
    DrawRender::registerTask(DRAW_RENDER);
    LOGI(OBF("HOOK_PLAY init done"));
}

}
