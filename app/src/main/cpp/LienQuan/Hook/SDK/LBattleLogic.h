#pragma once

#include <API/Il2CppApi.h>

namespace lienquan {

namespace LBattleLogic {

Object *Get();

bool HasActorMgr();
Object *GetGameActorMgr();
unsigned int GetHostPlayerId();
Object *GetHostActorRoot();

} // namespace LBattleLogic
} // namespace lienquan
