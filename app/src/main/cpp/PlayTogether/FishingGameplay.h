#pragma once

#include <API/Il2CppApi.h>

namespace FishingGameplay {
    void InitHooks();
    void TickPerfectTug(Object *player, Object *fishingSys, int fishingState);
    void TryFastBite(Object *player, int fishingState);
    void TryAutoEquipBait(Object *fishingSys);
    bool ShouldKeepCatch(unsigned int itemId, int grade);
}
