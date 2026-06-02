//
// Created by PC on 07/10/2025.
//

#include "MiniGameTowerOfHell.h"
#include "Tools/Tools.h"
#include "Config/Config.h"
#include "KinematicCharacterMotor.h"
#include "PlayLog.h"

namespace MiniGameTowerOfHell {
    Class *get_class() {
        return FindClass("PlayTogether.MiniGame.MiniGameTowerOfHell");
    }

    void (*old_Update)(Object *instance);

    void Update(Object *instance) {
        old_Update(instance);
        RATE_LIMIT(60);
        auto &towerClimb = gPLConfig.miniGame.towerClimb;
        if (!towerClimb.isEnable) return;
        static long long next_time = 0;
        static bool isWinner = false;
        if (next_time == 0) {

        } else if (Tools::getSystemMilliseconds() - next_time >= towerClimb.delayNextPoint + (rand() % 3 + 3) * 1000) { // wait next new point (random 3-5s)
            next_time = 0; // reset delay
        } else {
            return;
        }

        Object *manager = instance->get_field_object<Object *>("stageManager");
        if (!manager) return;

        List<Object *> *lists = manager->get_field_object<List<Object *> *>("stageObjectList");
        if (!lists) return;

        int curClearFloor = instance->get_field_value<int>("curClearFloor");
        if (curClearFloor + 2 < lists->get_Count()) {
            Object *item = lists->get_item(curClearFloor + 1);
            if (!item) return;

            Object *point = item->get_field_object<Object *>("<checkPoint>k__BackingField");
            if (!point) return;

            Object *activeRoot = point->get_field_object<Object *>("activeRoot");
            if (!activeRoot) return;

            Object *transformObj = activeRoot->invoke_method<Object *>("get_transform");
            if (!transformObj) return;

            if (next_time == 0) {
                LOGD("[MiniGameTowerOfHell] curClearFloor: %d, lists->get_Count: %d", curClearFloor, lists->get_Count());
                isWinner = false;
                next_time = Tools::getSystemMilliseconds();
                Vector3 pos = transformObj->invoke_method<Vector3>("get_position");
                pos.y += 3.0f;
                KinematicCharacterMotor::set_TransientPosition(pos);
            }
        } else if (!isWinner && next_time == 0) {
            isWinner = true;
            Vector3 pos(-11.276733, 64.994446, 19.976013);
            pos.y += 3.0f;
            KinematicCharacterMotor::set_TransientPosition(pos);
            LOGD("[MiniGameTowerOfHell] WINNER");
        }
    }

    void init() {
        Tools::Hook(get_class()->find_method("Update")->methodPointer, (void *) Update, (void **) &old_Update);
    }
}