#pragma once

#include <API/Il2CppApi.h>

namespace FishingGameplay {
    void InitHooks();
    void TickPerfectTug(Object *player, Object *fishingSys, int fishingState);
    void TryFastBite(Object *player, int fishingState);
    void TryAutoEquipBait(Object *fishingSys);
    void TickAuxMechanics(Object *player, Object *fishingSys, int fishingState, bool isFishing);
    void OnCastRecovered();
    void OnCastFailed(int castFailStreak, int lastFailType);
    bool TryCheckFishingPoint(Object *player);
    const char *GetStatusHint();
    bool ShouldKeepCatch(unsigned int itemId, int grade);
    bool CanPaceAct();
    bool IsRaidModeActive();
    bool GetEarlyCatchSellDecision(bool *outSell);
    void ClearEarlyCatchDecision();
    bool HasPendingRaid();
    unsigned int GetPendingRaidIdx();
    unsigned int GetCachedCastDifficultyId();
    bool QueryFishDifficulty(unsigned int sid, int *outShadowIndex, unsigned int *outDifficultyId);
    const char *ShadowLabelFromIndex(int index);
}
