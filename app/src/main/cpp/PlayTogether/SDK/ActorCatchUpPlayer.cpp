#include "PlayLog.h"
//
// Created by TEAMHMG on 14/09/2025.
//

#include "ActorCatchUpPlayer.h"
#include "Config/Config.h"

namespace ActorCatchUpPlayer {
    Class*get_class() {
        return FindClass("ActorCatchUpPlayer");
    }
    void set_AttackRange(Object*instance, float value) {
        if (!instance) {
            return;
        }
        instance->invoke_method<void>("set_AttackRange", value);
    }
    void set_AttackView(Object*instance, float value) {
        if (!instance) {
            return;
        }
        instance->invoke_method<void>("set_AttackView", value);
    }
    void (*old_OnUpdate)(Object*);
    void OnUpdate(Object*instance) {
        LOGI("ActorCatchUpPlayer::OnUpdate called");
        old_OnUpdate(instance);
        if (!instance || !gPLConfig.miniGame.zombie.isEnable) {
            return;
        }
        if (gPLConfig.miniGame.zombie.isChemXa) {
            set_AttackRange(instance, 99999);
            set_AttackView(instance, 99999);
        }
    }
}