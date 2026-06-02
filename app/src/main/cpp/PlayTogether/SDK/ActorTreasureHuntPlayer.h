//
// Created by TEAMHMG on 12/04/2026.
//

#pragma once
#ifndef IL2CPP_PLAY_ACTORTREASUREHUNTPLAYER_H
#define IL2CPP_PLAY_ACTORTREASUREHUNTPLAYER_H

#include <API/Il2CppApi.h>

namespace ActorTreasureHuntPlayer {
    Class *get_class();

    extern void (*old_OnUpdate)(Object *);
    void OnUpdate(Object *instance);
}

#endif //IL2CPP_PLAY_ACTORTREASUREHUNTPLAYER_H
