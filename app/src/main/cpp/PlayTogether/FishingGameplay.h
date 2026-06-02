#pragma once

#include <API/Il2CppApi.h>

namespace FishingGameplay {
    void InitHooks();
    void TickPerfectTug(Object *player, Object *fishingSys, int fishingState);
    void TryFastBite(Object *player, int fishingState);
    void TryAutoEquipBait(Object *fishingSys);
    bool ShouldKeepCatch(unsigned int itemId, int grade);
    bool CanPaceAct();
    bool IsRaidModeActive();
    bool GetEarlyCatchSellDecision(bool *outSell);
    void ClearEarlyCatchDecision();
    bool HasPendingRaid();
    unsigned int GetPendingRaidIdx();
}
