#pragma once

#include <API/Il2CppApi.h>

namespace FishingGameplay {
    void InitHooks();
    void TryAutoEquipBait(Object *fishingSys);
    bool ShouldKeepCatch(unsigned int itemId, int grade);
    bool GetEarlyCatchSellDecision(bool *outSell);
    void ClearEarlyCatchDecision();
    unsigned int GetCachedCastDifficultyId();
    bool QueryFishDifficulty(unsigned int sid, int *outShadowIndex, unsigned int *outDifficultyId);
    const char *ShadowLabelFromIndex(int index);
    int ShadowIndexFromAssetName(const char *name);
    bool RecordLearnedLevelFish(unsigned int levelId, unsigned int itemId);
}
