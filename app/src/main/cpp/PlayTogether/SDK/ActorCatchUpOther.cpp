//
// Created by TEAMHMG on 14/09/2025.
//

#include "ActorCatchUpOther.h"
#include "Config/Config.h"
#include "UnityEngine/Camera.h"
#include "UnityEngine/Component.h"
#include "UnityEngine/Transform.h"
#include "KinematicCharacterMotor.h"
#include "Stubs/ESPManager.h"
#include <imgui/imgui.h>
#include <Includes/obfuscate.h>
#define LOG_TAG OBF("ATTACK_PlayTogether")
#include <Includes/Logger.h>

namespace ActorCatchUpOther {
    Class *get_class() {
        return FindClass("ActorCatchUpOther");
    }


    void (*old_OnUpdate)(Object*);
    void OnUpdate(Object* instance){
        LOGI("ActorCatchUpOther::OnUpdate called");
        old_OnUpdate(instance);
        if (!instance || isGameLoading || !gPLConfig.miniGame.zombie.isEnable) return;

        UnityEngine::Camera* camera = UnityEngine::Camera::get_main();
        if (!camera || !camera->isValid()) {
            return;
        }
        UnityEngine::Component* pointer = instance->get_field_object<UnityEngine::Component*>("capsuleCollider");
        if (!pointer || !pointer->isValid()) {
            return;
        }
        UnityEngine::Transform* transform = pointer->get_transform();
        if (!transform || !transform->isValid()) {
            return;
        }
        Vector3 myPosition = KinematicCharacterMotor::get_TransientPosition();
        Vector3 myWorldPosition = camera->WorldToScreenPoint(myPosition);

        Vector3 position = transform->get_position();
        Vector3 worldPos = camera->WorldToScreenPoint(position);
        CatchUpPlayerRole_Type roleType = instance->invoke_method<CatchUpPlayerRole_Type>("get_PlayerRoleType");
        if (roleType == CatchUpPlayerRole_Type::None) {
            return;
        }

        ImVec4 color = ImVec4(1, 1, 1, 1);
        if (roleType == CatchUpPlayerRole_Type::Person) {
            color = ImVec4(0, 1, 0, 1); // Xanh lá cho người chơi bình thường
        } else {
            color = ImVec4(1, 0, 0, 1); // Đỏ cho người chơi nguy hiểm (Tagger hoặc InfectedPerson)
        }

        Vector2 myScreenPos2D = Vector2(myWorldPosition.x, ImGui::GetIO().DisplaySize.y - myWorldPosition.y);


        ESPEntry entry;
        entry.object = instance;
        entry.startPos = myScreenPos2D;
        entry.pos = position;
        entry.screenPos = worldPos;
        entry.color = color;
        entry.staleCount = 0;
        ESPManager::Add(entry);
    }
}