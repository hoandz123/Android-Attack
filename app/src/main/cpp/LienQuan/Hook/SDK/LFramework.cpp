#include "LFramework.h"
#include <Includes/obfuscate.h>

namespace lienquan {
namespace LFramework {

bool IsFramework(Object *desk) {
    if (!desk) return false;
    Class *fw = FindClass(OBF("NucleusDrive.Logic.LFramework"));
    Class *dc = desk->get_class();
    if (!fw || !dc) return false;
    return il2cpp_class_is_assignable_from(fw, dc);
}

bool IsStateSyncDesk(Object *desk) {
    if (!desk) return false;
    Class *ssd = FindClass(OBF("NucleusDrive.Logic.LStateSyncDesk"));
    Class *dc = desk->get_class();
    if (!ssd || !dc) return false;
    return il2cpp_class_is_assignable_from(ssd, dc);
}

Object *GetMainBattleLogic(Object *framework) {
    if (!framework) return nullptr;
    return framework->invoke_method<Object *>(OBF("get_mainBattleLogic"));
}

Object *GetDeskBattleLogic(Object *desk) {
    if (!desk) return nullptr;
    return desk->invoke_method<Object *>(OBF("get_DeskBattleLogic"));
}

Object *GetRealBattleLogic(Object *desk) {
    if (!desk) return nullptr;
    return desk->invoke_method<Object *>(OBF("get_realBattleLogic"));
}

} // namespace LFramework
} // namespace lienquan
