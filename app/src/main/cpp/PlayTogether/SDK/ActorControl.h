//
// Created by TEAMHMG on 13/09/2025.
//

#pragma once
#ifndef IL2CPP_PLAY_ACTORCONTROL_H
#define IL2CPP_PLAY_ACTORCONTROL_H
#include <API/Il2CppApi.h>

// Trạng thái loading dùng chung (bản gốc nằm trong Config). Port tối thiểu cho G2;
// ActorControl::get_Kunit set, KinematicCharacterMotor::UpdatePhase1 đọc (G4).
extern bool isGameLoading;

namespace ActorControl { //Nhân vật
    Class *get_class();
    extern Object *my_Unit;
    extern Object *my_Motor;
    extern Object *my_Player;

    extern Object *(*old_get_Kunit)(Object *instance);
    Object *get_Kunit(Object *instance);
}

#endif //IL2CPP_PLAY_ACTORCONTROL_H
