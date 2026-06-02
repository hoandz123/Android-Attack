//
// Created by TEAMHMG on 03/08/2025.
//

#pragma once
#ifndef PLAY_TREASUREBOX_STATE_H
#define PLAY_TREASUREBOX_STATE_H

enum class TreasureBox_State : int {
    None        = 0,
    Hide        = 1,
    Discover    = 2,
    Discover10  = 3,
    Discover20  = 4,
    Discover30  = 5,
    Discover40  = 6,
    Discover50  = 7,
    Discover60  = 8,
    Discover70  = 9,
    Discover80  = 10,
    Discover90  = 11,
    Complete    = 13,
    RewardOpen  = 14,
    Fade        = 15,
    End         = 16
};

#endif //PLAY_TREASUREBOX_STATE_H
