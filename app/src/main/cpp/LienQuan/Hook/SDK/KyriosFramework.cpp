#include "KyriosFramework.h"
#include <Includes/obfuscate.h>

namespace lienquan {
namespace KyriosFramework {

namespace {

Class *GetClass() {
    return FindClass(OBF("Kyrios.KyriosFramework"));
}

Object *GetInstance() {
    Class *cls = GetClass();
    if (!cls) return nullptr;
    Il2CppMethod *method = cls->find_method(OBF("GetInstance"), 0);
    if (!method) method = cls->find_method(OBF("get_instance"), 0);
    if (!method) return nullptr;
    return method->static_invoke<Object *>();
}

Object *GetHostLogic() {
    Class *cls = GetClass();
    if (!cls) return nullptr;
    Il2CppMethod *getHost = cls->find_method(OBF("get_hostLogic"), 0);
    if (!getHost) return nullptr;
    return getHost->static_invoke<Object *>();
}

} // namespace

bool GetIsRunning() {
    Object *inst = GetInstance();
    if (!inst) return false;
    return inst->invoke_method<bool>(OBF("get_IsRunning"));
}

int GetHostPlayerCamp() {
    Object *host = GetHostLogic();
    if (!host) return -1;
    return host->invoke_method<int>(OBF("get_hostPlayerCamp"));
}

} // namespace KyriosFramework
} // namespace lienquan
