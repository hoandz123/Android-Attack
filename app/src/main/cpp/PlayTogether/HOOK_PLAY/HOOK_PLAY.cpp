#include "HOOK_PLAY.h"
#include "AntiCheat.h"
#include "../SDK/FrameWork.h"
#include "../SDK/ActorControl.h"
#include "../SDK/KinematicCharacterMotor.h"
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
    Tools::Hook(ActorControl::get_class()->find_method(OBF("get_Kunit"), 0)->methodPointer, (void *) ActorControl::get_Kunit, (void **) &ActorControl::old_get_Kunit);
    Tools::Hook(KinematicCharacterMotor::get_class()->find_method(OBF("UpdatePhase1"), 1)->methodPointer, (void *) KinematicCharacterMotor::UpdatePhase1, (void **) &KinematicCharacterMotor::old_UpdatePhase1);
    // TODO G3: DialogUnit, DialogFishingGetItem, EquipmentSystem hooks
    // TODO G3: MapGuideArrow, HeadUpSelectButton, ActorCatchUp* hooks
    // TODO G3: DialogShopInGame, Treasure, ActorTreasureHuntPlayer hooks
    // TODO G4: MiniGameTowerOfHell, MiniGameObby, EventPickUpItemManager, MiniGameKMGUnit, TableFishingDifficultyImpl
    // TODO G4: FarmLandHooks, AutoTreasure
    // TODO G5: DRAW_RENDER + DrawRender::registerTask (ESP/mod-ui)
    // TODO G5: Config JSON slim via filemanager
    LOGI(OBF("HOOK_PLAY skeleton init done"));
}

} // namespace HOOK_PLAY
