//
// Created by TEAMHMG on 13/09/2025.
//

#pragma once
#ifndef IL2CPP_PLAY_MISSIONSYSTEM_H
#define IL2CPP_PLAY_MISSIONSYSTEM_H
#include <API/Il2CppApi.h>
#include "map"
#include "SystemHelper.h"

namespace MissionSystem {
    Class *get_class();
    Object *get_Instance();

    struct Achievement {
        int AchievementId;
        int Step;
        bool isComplete = false;
    };
    extern std::map<void *, Achievement> achievementList;

    void Update();
}


#endif //IL2CPP_PLAY_MISSIONSYSTEM_H
