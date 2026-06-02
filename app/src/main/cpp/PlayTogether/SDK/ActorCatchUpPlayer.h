//
// Created by TEAMHMG on 14/09/2025.
//

#pragma once
#ifndef IL2CPP_PLAY_ACTORCATCHUPPLAYER_H
#define IL2CPP_PLAY_ACTORCATCHUPPLAYER_H

#include <API/Il2CppApi.h>

namespace ActorCatchUpPlayer {
    Class* get_class();
    void set_AttackRange(Object* instance, float value);
    void set_AttackView(Object* instance, float value);
    extern void (*old_OnUpdate)(Object*);
    void OnUpdate(Object* instance);
}


#endif //IL2CPP_PLAY_ACTORCATCHUPPLAYER_H
