#pragma once

#include <API/Il2CppApi.h>
#include <API/Vector3.h>

namespace lienquan {

struct VInt3 {
    int x = 0;
    int y = 0;
    int z = 0;
};

namespace LActorRoot {

Object *FromPoolHandle(Object *poolHandle);
unsigned int GetObjID(Object *root);
unsigned int GetActorPlayerId(Object *root);
int GetEnemyCamp(Object *root);
Vector3 GetWorldPosition(Object *root);

int HeroCount(Object *gameActorMgr = nullptr);
Object *HeroAt(int index, Object *gameActorMgr = nullptr);

bool ReadActorMeta(Object *root, uint32_t &configId, uint32_t &skinId);

} // namespace LActorRoot
} // namespace lienquan
