#include "ActorManager.h"
#include <Includes/obfuscate.h>

namespace lienquan {
namespace ActorManager {

namespace {

Class *KyriosFrameworkClass() {
    static Class *cached = nullptr;
    if (!cached) cached = FindClass(OBF("Kyrios.KyriosFramework"));
    return cached;
}

Object *LinkerFromPoolHandle(Object *poolHandle) {
    if (!poolHandle) return nullptr;
    Object *linker = poolHandle->get_field_object<Object *>(OBF("_handleObj"));
    if (!linker || !linker->get_class()) return nullptr;
    if (linker->get_class()->get_name() != OBF("ActorLinker")) return nullptr;
    return linker;
}

} // namespace

Object *GetInstance() {
    Class *cls = KyriosFrameworkClass();
    if (!cls) return nullptr;
    Il2CppMethod *m = cls->find_method(OBF("get_actorManager"), 0);
    if (!m) return nullptr;
    return m->static_invoke<Object *>();
}

Object *HeroList(Object *mgr) {
    if (!mgr) mgr = GetInstance();
    if (!mgr) return nullptr;
    return mgr->get_field_object<Object *>(OBF("HeroActors"));
}

int HeroCount(Object *mgr) {
    Object *list = HeroList(mgr);
    return list ? list->invoke_method<int>(OBF("get_Count")) : 0;
}

Object *HeroHandleAt(int index, Object *mgr) {
    if (index < 0) return nullptr;
    Object *list = HeroList(mgr);
    if (!list) return nullptr;
    return list->invoke_method<Object *>(OBF("get_Item"), index);
}

Object *HeroLinkerAt(int index, Object *mgr) {
    return LinkerFromPoolHandle(HeroHandleAt(index, mgr));
}

} // namespace ActorManager
} // namespace lienquan
