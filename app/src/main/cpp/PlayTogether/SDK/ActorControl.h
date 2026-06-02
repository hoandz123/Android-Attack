//
// Created by TEAMHMG on 13/09/2025.
//

#pragma once
#ifndef IL2CPP_PLAY_ACTORCONTROL_H
#define IL2CPP_PLAY_ACTORCONTROL_H
#include <API/Il2CppApi.h>

namespace ActorControl {
    Class *get_class();
    extern Object *my_Unit;
    extern Object *my_Motor;
    extern Object *my_Player;
    extern Object *(*old_get_Kunit)(Object *instance);
    Object *get_Kunit(Object *instance);
    void GetListNPC();
}

#endif //IL2CPP_PLAY_ACTORCONTROL_H
