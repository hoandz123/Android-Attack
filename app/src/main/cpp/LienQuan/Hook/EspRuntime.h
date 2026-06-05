#pragma once

#include <API/Il2CppApi.h>
#include <cstdint>
#include <vector>

namespace lienquan {
namespace EspRuntime {

constexpr int kMaxTargets = 16;
constexpr int kMaxHeroIcons = 10;
constexpr int kKeyLen = 48;

struct LineItem {
    bool valid = false;
    float fromX = 0.f, fromY = 0.f, toX = 0.f, toY = 0.f;
};

struct MapItem {
    bool valid = false;
    unsigned int objId = 0;
    float x = 0.f, y = 0.f;
    char key[kKeyLen]{};
};

struct IconItem {
    bool valid = false;
    unsigned int objId = 0;
    char key[kKeyLen]{}; // HeroIcon::Entry::fieldName
};

struct Snapshot {
    bool valid = false;
    float screenW = 0.f, screenH = 0.f;
    bool hasMyWorld = false;
    int targetCount = 0;
    LineItem lines[kMaxTargets]{};
    MapItem mapItems[kMaxTargets]{};
    int iconCount = 0;
    IconItem icons[kMaxHeroIcons]{};
};

// gameActorMgr: LGameActorMgr instance from UpdateLogic hook (HeroActors + logic state).
void OnGameUpdate(Object *gameActorMgr);
// kyriosMgr: Kyrios.Actor.ActorManager from Update hook (HeroActors + get_ConfigId).
void OnActorManagerUpdate(Object *kyriosMgr);
bool ReadSnapshot(Snapshot &out);
bool GetIconPixels(const char *key, std::vector<uint8_t> &rgba, int &width, int &height, uint32_t &version);

} // namespace EspRuntime
} // namespace lienquan
