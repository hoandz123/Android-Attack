//
// Created by TEAMHMG on 13/09/2025.
//

#include "MapGuideArrow.h"
#include "Config/Config.h"
#include "KinematicCharacterMotor.h"

namespace MapGuideArrow { //Dịch chuyển NPC
    Class *get_class() {
        return FindClass("MapGuideArrow");
    }

    void (*old_Update)(Object *instance);
    void Update(Object *instance) {
        old_Update(instance);
        if (!instance || !gPLConfig.general.isTeleNpc) return;
        static int cacheMapId = 0;
        if (instance->get_field_value<bool>("arrowOn")) {
            Object *targetPoint = instance->get_field_object<Object *>("targetPoint");
            if (!targetPoint) return;
//            int currentMapId = CacheUser::myCurrentMapID();
//
//            int targetMapId = instance->get_field_value<int>("targetMapId");
//            if (targetMapId > 0) {
//                if (targetMapId != cacheMapId && targetMapId != currentMapId) {
//                    cacheMapId = targetMapId;
//                    LOGI("MapGuideArrow::Update - targetMapId: %d", targetMapId);
//                    LayerSystem::NextMap(targetMapId);
//                    return;
//                }
//                if (targetMapId != currentMapId) {
//                    LOGI("MapGuideArrow::Update - Waiting for map change... currentMapId: %d", currentMapId);
//                    return;
//                }
//            }

            Vector3 NewPos = targetPoint->invoke_method<Vector3>("get_position");
            if (NewPos != Vector3()) {
                NewPos.y += 2.0f;
                KinematicCharacterMotor::set_TransientPosition(NewPos);
                cacheMapId = 0;
            }
        } else {
            cacheMapId = 0;
        }
    }

}
