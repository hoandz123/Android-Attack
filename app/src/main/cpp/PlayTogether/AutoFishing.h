#pragma once

#include "SDK/enum/eFishingState.h"
#include <API/Vector3.h>
#include <string>

namespace AutoFishing {
    void Update();
    eFishingState GetLastFishingState();
    std::string GetStateLabel();
    int GetFishCaughtCount();
    int GetFailCount();
    int GetMissCount();
    int GetCatchCountByGrade(int grade);
    bool IsPausedByRare();
    bool HasRareAlert();
    void ClearRareAlert();
    unsigned int GetLastCatchItemId();
    int GetLastCatchGrade();
    unsigned int GetSessionElapsedSec();
    bool HasFloatPoint();
    Vector3 GetFloatPoint();
    unsigned int GetCastingZoneId();
    unsigned int GetCatchZoneId();
    long long GetFishingBaitUid();
    int GetLastFailType();
    bool IsFishingCountOver();
    int GetBigFishHp();
    int GetBigFishHpMax();
    int GetCastFailStreak();
    int GetCatchesPerHour();
    int GetSuccessRatePercent();
    void ResetSessionStats();
    unsigned int GetCurrentFishLevel();
    int GetCurrentShadowIndex();
    unsigned int GetCurrentDifficultyId();
}
