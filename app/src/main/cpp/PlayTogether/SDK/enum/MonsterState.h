//
// Created by TEAMHMG on 30/07/2025.
//

#pragma once
#ifndef PLAY_MONSTERSTATE_H
#define PLAY_MONSTERSTATE_H

#include <stdint.h>

enum class MonsterState : int32_t {
    None = 0,
    Ready = 1,
    Appear = 2,
    Default = 3,
    Dead = 4,
    DeadIdle = 5,
    Disappear = 6,
    Destroy = 7,
    Terminate = 8
};

#endif //PLAY_MONSTERSTATE_H
