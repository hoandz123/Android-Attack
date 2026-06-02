//
// Created by TEAMHMG on 14/09/2025.
//

#pragma once
#ifndef IL2CPP_PLAY_AUTOMONSTER_H
#define IL2CPP_PLAY_AUTOMONSTER_H
#include "API/Il2CppApi.h"

namespace Monster {

    enum class eAutoMonster {
        None = 0,
        FindMonster = 1,
        TeleMonster = 2,
        AutoAttack = 3,
        NextMap = 4,
        WaitingForNextMap = 5,
    };

    extern eAutoMonster curState;
    extern List<Object *> *currentList;
    extern Object *currentMonster;
    extern int targetMapId;

    Object *FindMonster();
    bool isValidMonster(Object *info);
    bool isTeleMonster(Object *info);

    void Update();
}

#endif //IL2CPP_PLAY_AUTOMONSTER_H
