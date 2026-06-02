#pragma once

#include "SDK/enum/eFishingState.h"
#include <API/Vector3.h>

namespace AutoFishing {
    void Update();
    unsigned int GetCastingZoneId();
    unsigned int GetCatchZoneId();
    long long GetFishingBaitUid();
    unsigned int GetCurrentFishLevel();
    int GetCurrentShadowIndex();
    unsigned int GetCurrentDifficultyId();
}
