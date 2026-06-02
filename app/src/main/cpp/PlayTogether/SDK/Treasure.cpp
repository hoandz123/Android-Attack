//
// Created by TEAMHMG on 14/09/2025.
//

#include "Treasure.h"
#include "Config/Config.h"
#include "UnityEngine/Camera.h"
#include "UnityEngine/Transform.h"
#include "enum/TreasureBox_State.h"
#include "KinematicCharacterMotor.h"
#include "Stubs/ESPManager.h"
#include "JsonConvert.h"
#include "NetNativeProtocol.h"
#include "Stubs/AutoTreasure.h"

namespace Treasure {
    Class *get_class() {
        return FindClass("PlayTogether.MiniGame.TreasureHunt.Treasure");
    }

    void (*old_Update)(Object* instance);
    void Update(Object* instance) {
        auto& digging = gPLConfig.miniGame.digging;
        if (!digging.isEnable || isGameLoading || !instance) {
            return;
        }
        Object *headUp = instance->get_field_object<Object *>("headUpBoxOpen");
		if (headUp) {
			headUp->invoke_method<void>("OnClick_BoxOpen");
		}
        UnityEngine::Transform *transform = instance->invoke_method<UnityEngine::Transform *>("get_transform");
        if (!transform || !transform->isValid()) return;
        Vector3 position = transform->get_position();

        TreasureBox_State stateCheck = instance->invoke_method<TreasureBox_State>("get_CurrentTreasureState");
        int uid = instance->invoke_method<int>("get_UID");
        int boxType = instance->invoke_method<int>("get_TreasureBoxType");

        if (stateCheck == TreasureBox_State::Hide) {
            AutoTreasure::OnTreasureScan(instance, position, uid, boxType);
        } else {
            AutoTreasure::RemoveTreasure(uid);
        }

        if (!digging.esp.isEnable) return;

        UnityEngine::Camera *camera = UnityEngine::Camera::get_main();
        if (!camera || !camera->isValid()) return;

        Vector3 screenPos = camera->WorldToScreenPoint(position);

        if (stateCheck != TreasureBox_State::Hide) return;

        ESPManager::Add(instance, Vector3(), screenPos);
    }

}

