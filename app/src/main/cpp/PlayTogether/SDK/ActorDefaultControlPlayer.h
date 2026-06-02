//
// Created by TEAMHMG on 13/09/2025.
//

#pragma once
#ifndef IL2CPP_PLAY_ACTORDEFAULTCONTROLPLAYER_H
#define IL2CPP_PLAY_ACTORDEFAULTCONTROLPLAYER_H
#include <cstdint>
#include <API/Il2CppApi.h>
#include "enum/eFishingState.h"

namespace ActorDefaultControlPlayer {
    Class *get_class();

    void Update();

    void Hook_SendToFishingCasting(void *thiz, void *fishingZoneList);
    extern void (*old_SendToFishingCasting)(void *, void *);

    void Hook_DialogZoneTitle_SetTitle(void *thiz, uint32_t titleID);
    extern void (*old_DialogZoneTitle_SetTitle)(void *, uint32_t);

}

#endif //IL2CPP_PLAY_ACTORDEFAULTCONTROLPLAYER_H
