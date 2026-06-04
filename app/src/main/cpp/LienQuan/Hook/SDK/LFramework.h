#pragma once

#include <API/Il2CppApi.h>

namespace lienquan {

namespace LFramework {

bool IsFramework(Object *desk);
bool IsStateSyncDesk(Object *desk);
Object *GetMainBattleLogic(Object *framework);
Object *GetDeskBattleLogic(Object *desk);
Object *GetRealBattleLogic(Object *desk);

} // namespace LFramework
} // namespace lienquan
