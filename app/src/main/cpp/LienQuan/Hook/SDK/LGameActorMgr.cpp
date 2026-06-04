#include "LGameActorMgr.h"
#include "LBattleLogic.h"
#include <Includes/obfuscate.h>

namespace lienquan {
namespace LGameActorMgr {

Object *GetInstance() {
    return LBattleLogic::GetGameActorMgr();
}

Object *HeroList(Object *mgr) {
    if (!mgr) mgr = GetInstance();
    if (!mgr) return nullptr;
    Object *heroes = mgr->get_field_object<Object *>(OBF("HeroActors"));
    if (heroes && heroes->invoke_method<int>(OBF("get_Count")) > 0) return heroes;
    return mgr->invoke_method<Object *>(OBF("GetAllHeros"));
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

} // namespace LGameActorMgr
} // namespace lienquan
