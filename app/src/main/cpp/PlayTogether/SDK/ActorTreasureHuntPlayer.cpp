//
// Created by TEAMHMG on 12/04/2026.
//

#include "ActorTreasureHuntPlayer.h"
#include "Config/Config.h"
#include "Stubs/AutoTreasure.h"

namespace ActorTreasureHuntPlayer {

    Class *get_class() {
        return FindClass("ActorTreasureHuntPlayer");
    }

    void (*old_OnUpdate)(Object *);
    void OnUpdate(Object *instance) {
        old_OnUpdate(instance);
        if (!instance) return;

        AutoTreasure::Update(instance);
    }

}
