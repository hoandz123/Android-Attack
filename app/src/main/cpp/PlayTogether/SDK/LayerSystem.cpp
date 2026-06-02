#include "PlayLog.h"
//
// Created by TEAMHMG on 13/09/2025.
//

#include "LayerSystem.h"
#include <Tools/Tools.h>
#include "CacheUser.h"
#include "KinematicCharacterMotor.h"
#include "DialogJoyStick.h"
#include "Config/Config.h"

void PLConfig::NextMapPos(int mapID, Vector3 pos) {
    LayerSystem::NextMapPos(mapID, pos);
}

namespace LayerSystem {
    int targetMapID = 0;
    int cacheMapID = 0;
    Vector3 targetPos = Vector3();
    long long lastMapChangeTime = 0;

    Class *get_class() {
        return FindClass("LayerSystem");
    }

    Object *get_Instance() {
        return SystemHelper::get_Layer();
    }

    void ConnectToZoneMove(int targetMapId, int fromType, String *serverName, bool useIris, bool useAdabt) {
        Object *instance = get_Instance();
        if (instance) {
            instance->invoke_method<void>("ConnectToZoneMove", targetMapId, fromType, serverName, useIris, useAdabt);
        } else {
            LOGE("LayerSystem::ConnectToZoneMove instance null");
        }
    }
    void NextMap(int targetMapId) {
        ConnectToZoneMove(targetMapId, 1, String::Create(""), true, false);
    }

    void NextMapPos(int mapID, Vector3 pos) {
        targetMapID = mapID;
        targetPos = pos;
    }
    void Update() {
        RATE_LIMIT(60);
        if (targetMapID > 0) {
            int currentMapId = CacheUser::myCurrentMapID();
            if (currentMapId != targetMapID) {
                if (cacheMapID != targetMapID) {
                    cacheMapID = targetMapID;
                    LOGI("LayerSystem::Update - NextMapPos to map ID: %d", targetMapID);
                    NextMap(targetMapID);
                }
            } else {
                LOGI("LayerSystem::Update - đã tới map ID: %d", targetMapID);
                if (cacheMapID > 0) lastMapChangeTime = Tools::getSystemMilliseconds();
                cacheMapID = 0;
                targetMapID = 0;
            }
        } else {
            if (targetPos != Vector3()) {
                if (lastMapChangeTime > 0 && Tools::getSystemMilliseconds() - lastMapChangeTime < 5000) {
                    return;
                }
                LOGI("LayerSystem::Update - Setting TransientPosition to (%f, %f, %f)", targetPos.x, targetPos.y, targetPos.z);
                DialogJoyStick::OnPress_JumpButton();
                KinematicCharacterMotor::set_TransientPosition(targetPos);
                targetPos = Vector3();
                lastMapChangeTime = 0;
            }
        }
    }
}