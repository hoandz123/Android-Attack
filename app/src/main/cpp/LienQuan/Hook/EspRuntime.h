#pragma once

#include "SDK/LActorRoot.h"
#include <API/Il2CppApi.h>
#include <cstdint>
#include <vector>

namespace lienquan {
namespace EspRuntime {

constexpr int kMaxTargets = 16;
constexpr int kMaxHeroIcons = 10;
constexpr int kKeyLen = 48;
constexpr int kInfoTextLen = 80;
constexpr int64_t kSnapshotStaleMs = 2000;

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

struct InfoItem {
    bool valid = false;
    float x = 0.f, y = 0.f;
    char text[kInfoTextLen]{};
    bool hasHp = false;
    int hp = 0;
    int maxHp = 0;
    bool isDead = false;
    bool lowHp = false;
    bool hasCooldowns = false;
    LActorRoot::SkillCooldownSlot cooldownSlots[LActorRoot::kSkillCooldownSlots]{};
};

struct ArrowItem {
    bool valid = false;
    float x = 0.f, y = 0.f;
    float angle = 0.f;
};

struct Snapshot {
    bool valid = false;
    int64_t updatedMs = 0;
    float screenW = 0.f, screenH = 0.f;
    bool hasMyWorld = false;
    int targetCount = 0;
    LineItem lines[kMaxTargets]{};
    MapItem mapItems[kMaxTargets]{};
    int iconCount = 0;
    IconItem icons[kMaxHeroIcons]{};
    InfoItem infoItems[kMaxTargets]{};
    ArrowItem arrows[kMaxTargets]{};
};

// gameActorMgr: LGameActorMgr instance from UpdateLogic hook (HeroActors + logic state).
void OnGameUpdate(Object *gameActorMgr);
// kyriosMgr: Kyrios.Actor.ActorManager from Update hook (HeroActors + get_ConfigId).
void OnActorManagerUpdate(Object *kyriosMgr);
bool ReadSnapshot(Snapshot &out);
bool GetIconPixels(const char *key, std::vector<uint8_t> &rgba, int &width, int &height, uint32_t &version);

} // namespace EspRuntime
} // namespace lienquan
