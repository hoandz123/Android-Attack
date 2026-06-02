#pragma once
#ifndef IL2CPP_PLAY_FISHINGSYSTEM_H
#define IL2CPP_PLAY_FISHINGSYSTEM_H
#include <API/Il2CppApi.h>
#include "SystemHelper.h"

namespace FishingSystem {
    extern int MagicWaterLeft;
    Class *get_class();

    Object *get_Instance();

    void Update();

    int GetActiveBuffCount(int itemID);
}

#endif //IL2CPP_PLAY_FISHINGSYSTEM_H
