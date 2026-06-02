//
// Created by TEAMHMG on 13/09/2025.
//

#pragma once
#ifndef IL2CPP_PLAY_KINEMATICCHARACTERMOTOR_H
#define IL2CPP_PLAY_KINEMATICCHARACTERMOTOR_H


#include <API/Il2CppApi.h>


namespace KinematicCharacterMotor { //Dịch chuyển
    Class *get_class();

    Object *get_transform();
    Vector3 get_TransientPosition();
    void set_TransientPosition(Vector3 position);


    extern void (*old_UpdatePhase1)(Object *instance, float deltaTime);
    void UpdatePhase1(Object *instance, float deltaTime);

}

#endif //IL2CPP_PLAY_KINEMATICCHARACTERMOTOR_H
