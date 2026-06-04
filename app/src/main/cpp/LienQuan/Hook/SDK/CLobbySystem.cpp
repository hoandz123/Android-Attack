#include "CLobbySystem.h"
#include <Includes/obfuscate.h>

namespace lienquan {
namespace CLobbySystem {

Class *get_class() {
    return FindClass(OBF("Assets.Scripts.GameSystem.CLobbySystem"));
}

Object *get_Instance() {
    Class *cls = get_class();
    if (!cls) return nullptr;
    Il2CppMethod *method = cls->find_method(OBF("GetInstance"), 0);
    if (!method) method = cls->find_method(OBF("get_instance"), 0);
    if (!method) return nullptr;
    return method->static_invoke<Object *>();
}

} // namespace CLobbySystem
} // namespace lienquan
