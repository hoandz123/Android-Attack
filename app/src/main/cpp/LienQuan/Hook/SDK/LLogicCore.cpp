#include "LLogicCore.h"
#include <Includes/obfuscate.h>

namespace lienquan {
namespace LLogicCore {

Object *GetInstance() {
    Class *cls = FindClass(OBF("NucleusDrive.Logic.LLogicCore"));
    if (!cls) return nullptr;
    Il2CppMethod *getInst = cls->find_method(OBF("GetInstance"), 0);
    if (!getInst) getInst = cls->find_method(OBF("get_instance"), 0);
    if (!getInst) return nullptr;
    return getInst->static_invoke<Object *>();
}

Object *GetCurUpdatingDesk() {
    Object *instance = GetInstance();
    if (!instance) return nullptr;
    return instance->invoke_method<Object *>(OBF("get_curUpdatingDesk"));
}

Object *GetInstances() {
    Object *instance = GetInstance();
    if (!instance) return nullptr;
    return instance->get_field_object<Object *>(OBF("instances"));
}

int DeskCount() {
    Object *list = GetInstances();
    return list ? list->invoke_method<int>(OBF("get_Count")) : 0;
}

Object *DeskAt(int index) {
    Object *list = GetInstances();
    if (!list) return nullptr;
    return list->invoke_method<Object *>(OBF("get_Item"), index);
}

} // namespace LLogicCore
} // namespace lienquan
