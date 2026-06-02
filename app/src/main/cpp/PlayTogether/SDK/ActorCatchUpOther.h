//
// Created by TEAMHMG on 14/09/2025.
//

#pragma once
#ifndef IL2CPP_PLAY_ACTORCATCHUPOTHER_H
#define IL2CPP_PLAY_ACTORCATCHUPOTHER_H
#include <API/Il2CppApi.h>

namespace ActorCatchUpOther {
    Class* get_class();

    enum class CatchUpPlayerRole_Type {
        None = 0,
        Tagger = 1,
        InfectedPerson = 2,
        Person = 3,
        End = 4,
    };


    extern void (*old_OnUpdate)(Object*);
    void OnUpdate(Object* instance);
}


#endif //IL2CPP_PLAY_ACTORCATCHUPOTHER_H
