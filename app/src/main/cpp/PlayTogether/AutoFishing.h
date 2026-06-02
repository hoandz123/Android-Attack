#pragma once

#include "SDK/enum/eFishingState.h"
#include <string>

namespace AutoFishing {
    void Update();
    eFishingState GetLastFishingState();
    std::string GetStateLabel();
    int GetFishCaughtCount();
}
