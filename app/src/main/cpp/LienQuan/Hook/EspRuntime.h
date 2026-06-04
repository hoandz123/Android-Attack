#pragma once

#include "SDK/MinimapSys.h"
#include <API/Il2CppApi.h>
#include <API/Vector3.h>
#include <cstdint>
#include <string>
#include <vector>

namespace lienquan {
namespace EspRuntime {

constexpr int kMaxTargets = 16;
constexpr int kMaxStripIcons = 10;
constexpr int kKeyLen = 48;

struct LineItem {
    bool valid = false;
    float fromX = 0.f, fromY = 0.f, toX = 0.f, toY = 0.f;
};

struct MapItem {
    bool valid = false;
    unsigned int objId = 0;
    int camp = -1;
    float x = 0.f, y = 0.f;
};

struct IconItem {
    bool valid = false;
    unsigned int objId = 0;
    float x = 0.f, y = 0.f, size = 0.f;
    char key[kKeyLen]{}; // HeroIcon::Entry::fieldName
};

struct Snapshot {
    uint32_t seq = 0;
    bool valid = false;
    float screenW = 0.f, screenH = 0.f;
    bool hasMyWorld = false;
    int myCamp = -1;
    int myEnemyCamp = -1;
    unsigned int dbgHostPid = 0;
    unsigned int dbgMetaPid = 0;
    MinimapSys::EMapType mapMode = MinimapSys::EMapType::None;
    int targetCount = 0;
    Vector3 targetWorld[kMaxTargets]{};
    int targetCamp[kMaxTargets]{};
    unsigned int targetObjId[kMaxTargets]{};
    LineItem lines[kMaxTargets]{};
    MapItem mapItems[kMaxTargets]{};
    int iconCount = 0;
    IconItem icons[kMaxStripIcons]{};
};

// gameActorMgr: LGameActorMgr instance from UpdateLogic hook (HeroActors + logic state).
void OnGameUpdate(Object *gameActorMgr);
// kyriosMgr: Kyrios.Actor.ActorManager from Update hook (HeroActors + get_ConfigId).
void OnActorManagerUpdate(Object *kyriosMgr);
bool ReadSnapshot(Snapshot &out);
bool GetIconPixels(const char *key, std::vector<uint8_t> &rgba, int &width, int &height, uint32_t &version);

} // namespace EspRuntime
} // namespace lienquan
