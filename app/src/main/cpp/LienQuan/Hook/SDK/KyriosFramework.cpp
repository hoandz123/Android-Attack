#include "KyriosFramework.h"
#include <Includes/obfuscate.h>

namespace lienquan {
namespace KyriosFramework {

Class *get_class() {
    return FindClass(OBF("Kyrios.KyriosFramework"));
}

Object *get_Instance() {
    Class *cls = get_class();
    if (!cls) return nullptr;
    Il2CppMethod *method = cls->find_method(OBF("GetInstance"), 0);
    if (!method) method = cls->find_method(OBF("get_instance"), 0);
    if (!method) return nullptr;
    return method->static_invoke<Object *>();
}

bool get_IsRunning() {
    Object *inst = get_Instance();
    if (!inst) return false;
    return inst->invoke_method<bool>(OBF("get_IsRunning"));
}

Object *get_actorManager() {
    Class *cls = get_class();
    if (!cls) return nullptr;
    Il2CppMethod *method = cls->find_method(OBF("get_actorManager"), 0);
    if (!method) return nullptr;
    return method->static_invoke<Object *>();
}

} // namespace KyriosFramework
} // namespace lienquan
