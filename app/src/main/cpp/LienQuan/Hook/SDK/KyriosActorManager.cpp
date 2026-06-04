#include "KyriosActorManager.h"
#include "KyriosFramework.h"
#include <Includes/obfuscate.h>

namespace lienquan {
namespace KyriosActorManager {

Class *get_class() {
    return FindClass(OBF("Kyrios.Actor.ActorManager"));
}

Object *get_Instance() {
    return KyriosFramework::get_actorManager();
}

int get_HeroActorCount() {
    Object *mgr = get_Instance();
    if (!mgr) return -1;
    Object *heroList = mgr->get_field_object<Object *>(OBF("HeroActors"));
    if (!heroList) return -1;
    return heroList->invoke_method<int>(OBF("get_Count"));
}

} // namespace KyriosActorManager
} // namespace lienquan
