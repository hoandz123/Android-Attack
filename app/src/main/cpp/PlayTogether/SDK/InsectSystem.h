//
// Created by TEAMHMG on 13/09/2025.
//

#pragma once
#ifndef IL2CPP_PLAY_INSECTSYSTEM_H
#define IL2CPP_PLAY_INSECTSYSTEM_H
#include <API/Il2CppApi.h>
#include "SystemHelper.h"

namespace InsectSystem {
    Class *get_class();
    Object *get_Instance();
    float get_distance_catch();

    void Update();
}

#endif //IL2CPP_PLAY_INSECTSYSTEM_H
