#pragma once

#include "SDK/enum/eFishingState.h"
#include <string>

namespace AutoFishing {
    void Update();
    std::string GetStateLabel();
    int GetFishCaughtCount();
}
