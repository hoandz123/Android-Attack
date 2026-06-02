#include "HOOK_PLAY.h"
#include "AntiCheat.h"
#include "../SDK/FrameWork.h"
#include "../SDK/ActorControl.h"
#include "../SDK/KinematicCharacterMotor.h"
#include "../SDK/DialogUnit.h"
#include "../SDK/DialogFishingGetItem.h"
#include "../SDK/EquipmentSystem.h"
#include "../SDK/MapGuideArrow.h"
#include "../SDK/HeadUpSelectButton.h"
#include "../SDK/ActorCatchUpPlayer.h"
#include "../SDK/ActorCatchUpOther.h"
#include "../SDK/DialogShopInGame.h"
#include "../SDK/Treasure.h"
#include "../SDK/ActorTreasureHuntPlayer.h"
#include <API/Il2CppApi.h>
#include <Includes/obfuscate.h>
#define LOG_TAG OBF("AttackPlugin")
#include <Includes/Logger.h>
#include <Tools/Tools.h>
#include <thread>

namespace HOOK_PLAY {

void init() {
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
            } else {
                LOGE(OBF("FrameWork::get_Instance() not found"));
            }
        }
    }).detach();
    Tools::Hook(DialogUnit::get_class()->find_method(OBF("DialogShow"), 0)->methodPointer, (void *) DialogUnit::DialogShow, (void **) &DialogUnit::old_DialogShow);
    Tools::Hook(DialogFishingGetItem::get_class()->find_method(OBF("OnItemSell"), 1)->methodPointer, (void *) DialogFishingGetItem::OnItemSell, (void **) &DialogFishingGetItem::old_OnItemSell);
    Tools::Hook(EquipmentSystem::get_class()->find_method(OBF("ShowRepairItem"), 2)->methodPointer, (void *) EquipmentSystem::ShowRepairItem, (void **) &EquipmentSystem::old_ShowRepairItem);
    Tools::Hook(ActorControl::get_class()->find_method(OBF("get_Kunit"), 0)->methodPointer, (void *) ActorControl::get_Kunit, (void **) &ActorControl::old_get_Kunit);
    Tools::Hook(KinematicCharacterMotor::get_class()->find_method(OBF("UpdatePhase1"), 1)->methodPointer, (void *) KinematicCharacterMotor::UpdatePhase1, (void **) &KinematicCharacterMotor::old_UpdatePhase1);
    Tools::Hook(MapGuideArrow::get_class()->find_method(OBF("Update"), 0)->methodPointer, (void *) MapGuideArrow::Update, (void **) &MapGuideArrow::old_Update);
    Tools::Hook(HeadUpSelectButton::get_class()->find_method(OBF("SetSprite"), 1)->methodPointer, (void *) HeadUpSelectButton::SetSprite, (void **) &HeadUpSelectButton::old_SetSprite);
    Tools::Hook(HeadUpSelectButton::get_class()->find_method(OBF("UpdatePosition"), 0)->methodPointer, (void *) HeadUpSelectButton::UpdatePosition, (void **) &HeadUpSelectButton::old_UpdatePosition);
    Tools::Hook(ActorCatchUpPlayer::get_class()->find_method(OBF("OnUpdate"), 0)->methodPointer, (void *) ActorCatchUpPlayer::OnUpdate, (void **) &ActorCatchUpPlayer::old_OnUpdate);
    Tools::Hook(ActorCatchUpOther::get_class()->find_method(OBF("OnUpdate"), 0)->methodPointer, (void *) ActorCatchUpOther::OnUpdate, (void **) &ActorCatchUpOther::old_OnUpdate);
    Tools::Hook(DialogShopInGame::get_class()->find_method(OBF("Update"), 0)->methodPointer, (void *) DialogShopInGame::Update, (void **) &DialogShopInGame::old_Update);
    Tools::Hook(Treasure::get_class()->find_method(OBF("Update"), 0)->methodPointer, (void *) Treasure::Update, (void **) &Treasure::old_Update);
    Tools::Hook(ActorTreasureHuntPlayer::get_class()->find_method(OBF("OnUpdate"), 0)->methodPointer, (void *) ActorTreasureHuntPlayer::OnUpdate, (void **) &ActorTreasureHuntPlayer::old_OnUpdate);
    // TODO G4: MiniGameTowerOfHell, MiniGameObby, EventPickUpItemManager, MiniGameKMGUnit, TableFishingDifficultyImpl::init
    // TODO G4: FarmLandHooks, AutoTreasure logic, NetNativeProtocol fishing cast hooks
    // TODO G5: DRAW_RENDER + DrawRender::registerTask (ESP/mod-ui)
    // TODO G5: Config JSON load/save via filemanager
    LOGI(OBF("HOOK_PLAY G3 init done"));
}

}
