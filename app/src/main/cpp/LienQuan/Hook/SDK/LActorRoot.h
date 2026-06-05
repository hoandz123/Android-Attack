#pragma once

#include <API/Il2CppApi.h>
#include <API/Vector3.h>
#include <cstdint>
#include <string>

namespace lienquan {

struct VInt3 {
    int x = 0;
    int y = 0;
    int z = 0;
};

namespace LActorRoot {

constexpr int kSkillCooldownSlots = 4;

struct SkillCooldownSlot {
    bool valid = false;
    bool unlocked = false;
    bool ready = false;
    int cdSec = 0;
};

struct SkillCooldownInfo {
    bool hasData = false;
    SkillCooldownSlot slots[kSkillCooldownSlots]{};
};

struct ActorInfo {
    unsigned int objId = 0;
    unsigned int playerId = 0;
    uint32_t configId = 0;
    uint32_t skinId = 0;
    int actorCamp = -1;
    int enemyCamp = -1;
    Vector3 worldPos{};
    int hp = 0;
    int maxHp = 0;
    int soulLevel = 0;
    bool isDead = false;
    bool hasCamp = false;
    bool hasHp = false;
    bool hasSoulLevel = false;
    bool hasDeadState = false;
    std::string actorName;
};

Object *FromPoolHandle(Object *poolHandle);
unsigned int GetObjID(Object *root);
unsigned int GetActorPlayerId(Object *root);
int GetActorCamp(Object *root);
int GetEnemyCamp(Object *root);
Vector3 GetWorldPosition(Object *root);
bool IsActorDead(Object *root);
bool ReadActorHp(Object *root, int &hp, int &maxHp);
bool ReadActorSoulLevel(Object *root, int &level);
bool ReadActorMeta(Object *root, uint32_t &configId, uint32_t &skinId);
bool ReadActorInfo(Object *root, ActorInfo &out);
bool ReadSkillCooldowns(Object *root, SkillCooldownInfo &out);
int SkillSlotTypeOf(const char *slotName);
bool ReadSkillSlot(Object *root, int slotType, SkillCooldownSlot &out);

int HeroCount(Object *gameActorMgr = nullptr);
Object *HeroAt(int index, Object *gameActorMgr = nullptr);

} // namespace LActorRoot
} // namespace lienquan
