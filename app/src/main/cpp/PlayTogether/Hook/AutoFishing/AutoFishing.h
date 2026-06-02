#pragma once

#include "PickerSnapshot.h"
#include <API/Il2CppApi.h>

namespace AutoFishing {
    void Update();
    unsigned int GetCastingZoneId();
    unsigned int GetCatchZoneId();
    long long GetFishingBaitUid();
    unsigned int GetCurrentFishLevel();

    void onReceiveFishingCatch(Object *self, Object *rewardInfo);
    void onPID_FISHING_CASTING(Object *self, Object *protocol);
    extern void (*old_ReceiveFishingCatch)(Object *, Object *);
    extern void (*old_PID_FISHING_CASTING)(Object *, Object *);
}
