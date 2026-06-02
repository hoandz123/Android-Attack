#pragma once

#include <API/Vector3.h>

namespace OverlaySnapshot {

struct View {
    bool ready = false;
    int mapId = 0;
    Vector3 position{};
    int fishCaught = 0;
    int fishingState = 0;
    int failCount = 0;
    int missCount = 0;
    int catchGrade1 = 0;
    int catchGrade2 = 0;
    int catchGrade3 = 0;
    int catchGrade4 = 0;
    int catchGrade5 = 0;
    unsigned int sessionSec = 0;
    unsigned int lastCatchItemId = 0;
    int lastCatchGrade = 0;
    unsigned int castingZoneId = 0;
    unsigned int catchZoneId = 0;
    long long baitUid = 0;
    bool pausedByRare = false;
    bool rareAlert = false;
    bool hasFloatPoint = false;
    float floatOffsetX = 0.f;
    float floatOffsetZ = 0.f;
    float floatDistance = 0.f;
    float floatBearingDeg = 0.f;
    int lastFailType = 0;
    bool fishingCountOver = false;
    int bigFishHp = 0;
    int bigFishHpMax = 0;
    int castFailStreak = 0;
    int catchesPerHour = 0;
    int successRatePct = 0;
};

void UpdateFromGameThread();
void Read(View &out);
const char *FishingStateLabel(int fishingState);
const char *FailTypeLabel(int failType);
const char *GradeLabel(int grade);

}
