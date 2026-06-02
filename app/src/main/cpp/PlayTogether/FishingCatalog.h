#pragma once

#include <atomic>
#include <cstring>

namespace FishingCatalog {

constexpr int kMaxBaits = 64;
constexpr int kMaxZones = 128;
constexpr int kMaxGuides = 128;
constexpr int kMaxFish = 300;
constexpr int kMaxLevels = 64;
constexpr int kMaxLearnedFishPerLevel = 16;
constexpr int kLabelLen = 96;

struct BaitEntry {
    unsigned int itemId = 0;
    long long uid = 0;
    int count = 0;
    int grade = 0;
    bool equipped = false;
    bool locked = false;
    char label[kLabelLen]{};
};

struct ZoneEntry {
    unsigned int zoneId = 0;
    unsigned int actionId = 0;
    bool isCurrent = false;
    char label[kLabelLen]{};
};

struct GuideEntry {
    int guidePointId = 0;
    unsigned int zoneId = 0;
    bool isActive = false;
    bool onCurrentMap = false;
    char label[kLabelLen]{};
};

struct FishEntry {
    unsigned int itemId = 0;
    int grade = 0;
    int fishType = 0;
    bool inCodex = false;
    char label[kLabelLen]{};
};

struct LevelEntry {
    unsigned int levelId = 0;
    int shadowIndex = 0;
    bool bigFish = false;
    float minigameHp = 0.f;
    char label[kLabelLen]{};
    int learnedFishCount = 0;
    char learnedFish[kMaxLearnedFishPerLevel][kLabelLen]{};
};

struct Snapshot {
    bool ready = false;
    int mapId = 0;
    int baitCount = 0;
    BaitEntry baits[kMaxBaits]{};
    int zoneCount = 0;
    ZoneEntry zones[kMaxZones]{};
    int guideCount = 0;
    GuideEntry guides[kMaxGuides]{};
    int fishCount = 0;
    FishEntry fish[kMaxFish]{};
    int levelCount = 0;
    LevelEntry levels[kMaxLevels]{};
};

void UpdateFromGameThread();
void RequestRebuild();
void NotifyPickerOpen();
void Read(Snapshot &out);

const char *GradeTag(int grade);
const char *FishTypeTag(int fishType);

}
