#include "LGameActorMgr.h"
#include "LBattleLogic.h"
#include <Includes/obfuscate.h>

namespace lienquan {
namespace LGameActorMgr {

namespace {

bool PtrLooksValid(Object *o) {
    if (!o) return false;
    const auto u = reinterpret_cast<uintptr_t>(o);
    if ((u & 3) != 0 || u <= 0x10000) return false;
    return true;
}

bool ManagedOk(Object *o) {
    if (!PtrLooksValid(o)) return false;
    try {
        return o->get_class() != nullptr;
    } catch (...) {
        return false;
    }
}

Object *FieldList(Object *mgr, const char *field) {
    if (!ManagedOk(mgr)) return nullptr;
    try {
        Class *cls = mgr->get_class();
        if (!cls || !cls->get_field_from_name(field)) return nullptr;
        return mgr->get_field_object<Object *>(field);
    } catch (...) {
        return nullptr;
    }
}

Object *MethodList(Object *mgr, const char *method) {
    if (!ManagedOk(mgr)) return nullptr;
    try {
        Class *cls = mgr->get_class();
        if (!cls || !cls->find_method(method, 0)) return nullptr;
        return mgr->invoke_method<Object *>(method);
    } catch (...) {
        return nullptr;
    }
}

int ListCount(Object *list) {
    if (!ManagedOk(list)) return 0;
    try {
        const int n = list->get_field_value<int>(OBF("_size"));
        if (n >= 0 && n <= 256) return n;
    } catch (...) {
    }
    try {
        Class *cls = list->get_class();
        if (!cls || !cls->find_method(OBF("get_Count"), 0)) return 0;
        const int n = list->invoke_method<int>(OBF("get_Count"));
        if (n < 0 || n > 256) return 0;
        return n;
    } catch (...) {
        return 0;
    }
}

Object *ListAt(Object *list, int index) {
    if (index < 0 || !ManagedOk(list)) return nullptr;
    const int n = ListCount(list);
    if (index >= n) return nullptr;
    try {
        Class *cls = list->get_class();
        if (!cls || !cls->find_method(OBF("get_Item"), 1)) return nullptr;
        return list->invoke_method<Object *>(OBF("get_Item"), index);
    } catch (...) {
        return nullptr;
    }
}

} // namespace

Object *GetInstance() {
    return LBattleLogic::GetGameActorMgr();
}

Object *HeroList(Object *mgr) {
    if (!mgr) mgr = GetInstance();
    Object *heroes = FieldList(mgr, OBF("HeroActors"));
    if (ListCount(heroes) > 0) return heroes;
    return MethodList(mgr, OBF("GetAllHeros"));
}

int HeroCount(Object *mgr) {
    return ListCount(HeroList(mgr));
}

Object *HeroHandleAt(int index, Object *mgr) {
    if (index < 0) return nullptr;
    return ListAt(HeroList(mgr), index);
}

} // namespace LGameActorMgr
} // namespace lienquan
