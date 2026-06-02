//
// Created by PC on 07/10/2025.
//

#include "MiniGameObby.h"
#include "Tools/Tools.h"
#include "Config/Config.h"
#include "KinematicCharacterMotor.h"
#include "EventPickUpItemManager.h"
#include "PlayLog.h"

namespace MiniGameObby {
    Class *get_class() {
        return FindClass("PlayTogether.MiniGame.MiniGameObby");
    }

    enum class eObbyGameType {
        Obby = 0,
        MoveUp = 1
    };

    void (*old_Update)(...);

    void Update(Object *instance) {
        old_Update(instance);
        RATE_LIMIT(60);
        auto &obby = gPLConfig.miniGame.obby;
        auto &ThapGa = gPLConfig.miniGame.ThapGa;
        if ((!obby.isEnable && !ThapGa.isEnable) || !EventPickUpItemManager::get_instance()) return;

        eObbyGameType gameType = instance->get_field_value<eObbyGameType>("_gameType");

        static bool isNhatLong = false;
        static int cacheNextLongGa = -1;
        if (gameType == eObbyGameType::MoveUp && !isNhatLong && ThapGa.isEnable) {
            List<Object *> *longga = EventPickUpItemManager::get_allPickupItems();
            if (longga && longga->get_Count() > 0) {
                RATE_LIMIT(1000);
                for (int i = 0; i < longga->get_Count(); i++) {
                    auto item = longga->get_item(i);
                    LOGI("longga item: %p", item);
                    if (!item) continue;
                    if (!item->get_field_object<Object*>("_collectMission")) continue;
                    if (item->get_field_value<bool>("_isEnter")) {
                        Object *headup = item->get_field_object<Object *>("_headUpButton");
                        if (headup) {
                            headup->invoke_method<void>("OnClick");
                            cacheNextLongGa = i;
                            LOGI("cacheNextLongGa OnClick: %d", cacheNextLongGa);
                            return;
                        }
                    } else {
                        if (i <= cacheNextLongGa) {
                            continue;
                        }
                        Object *ModelRoot = item->get_field_object<Object *>("ModelRoot");
                        Object *OnClickCB = item->get_field_object<Object *>("OnClickCB");
                        if (ModelRoot && OnClickCB) {
                            Vector3 pos = ModelRoot->invoke_method<Vector3>("get_position");
                            pos.y += 1.0f;
                            KinematicCharacterMotor::set_TransientPosition(pos);
                            LOGI("cacheNextLongGa: %d i = %d", cacheNextLongGa, i);
                            LOGD("set_TransientPosition: %f, %f, %f", pos.x, pos.y, pos.z);
                            return;
                        } else {
                            continue;
                        }
                    }
                }
                LOGD("isNhatLong = true;");
                isNhatLong = true;
                return;
            } else {
                LOGE("longga null or count 0");
            }
        }

        List<Object *> *boxRewardDatas = instance->get_field_object<List<Object *> *>("boxRewardDatas"); // tổng các points trong game
        if (!boxRewardDatas) return;
        for (int i = 0; i < boxRewardDatas->get_Count(); i++) {
            auto item = boxRewardDatas->get_item(i);
            if (!item) continue;
            Object *cacheHeadup = item->get_field_object<Object *>("cacheHeadup");
            if (cacheHeadup) cacheHeadup->invoke_method<void>("OnClick_BoxOpen");
        }

        static long long next_time = 0;
        if (next_time == 0) {
        } else if (Tools::getSystemMilliseconds() - next_time >= (obby.isEnable ? obby.delayNextPoint : ThapGa.delayNextPoint) + (rand() % 3 + 3) * 1000) { // wait next new point (random 3-5s)
            next_time = 0; // reset delay
        } else {
            return;
        }

        auto manager = instance->invoke_method<Object *>("get_obbyStageManager");
        if (!manager) return;

        List<Object *> *points = manager->get_field_object<List<Object *> *>("_checkPointList"); // tổng các points trong game
        if (!points) return;

        int curCheckPointIndex = manager->get_field_value<int>("curCheckPointIndex");
        if (curCheckPointIndex + 1 < points->get_Count()) {
            auto item = points->get_item(curCheckPointIndex + 1);
            if (!item) return;
            if (next_time == 0) {
                next_time = Tools::getSystemMilliseconds();

                Object *activeRoot = item->get_field_object<Object *>("_cacheGameObject");
                if (!activeRoot) return;

                Object *transformObj = activeRoot->invoke_method<Object *>("get_transform");
                if (!transformObj) return;

                Vector3 pos = transformObj->invoke_method<Vector3>("get_position");
                pos.y += item->get_field_value<float>("RespawnHeight");
                KinematicCharacterMotor::set_TransientPosition(pos);

            }
        }
    }

    void (*old_ConnectToBattleMove)(...);

    void ConnectToBattleMove(Object *instance, String *battleMove) {
        old_ConnectToBattleMove(instance, battleMove);
        LOGI("MiniGameObby::ConnectToBattleMove %s", battleMove ? battleMove->to_string().c_str() : "null");
    }

    void init() {
        Tools::Hook(get_class()->find_method("Update")->methodPointer, (void *) Update, (void **) &old_Update);
        Tools::Hook(FindClass("LayerSystem")->find_method("ConnectToBattleMove", 1)->methodPointer, (void *) ConnectToBattleMove, (void **) &old_ConnectToBattleMove);
    }
}