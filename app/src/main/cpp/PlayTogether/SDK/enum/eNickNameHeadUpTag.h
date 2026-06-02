//
// Created by TEAMHMG on 26/07/2025.
//

#pragma once
#ifndef PLAY_ENICKNAMEHEADUPTAG_H
#define PLAY_ENICKNAMEHEADUPTAG_H

#include <stdint.h>

enum class eNickNameHeadUpTag : uint32_t {
    None         = 0,
    Player       = 2,
    OtherPlayer  = 4,
    NPC          = 8,
    Pet          = 16,
    Monster      = 32,
    Baby         = 64,
    All          = 4294967295u
};

#endif //PLAY_ENICKNAMEHEADUPTAG_H
