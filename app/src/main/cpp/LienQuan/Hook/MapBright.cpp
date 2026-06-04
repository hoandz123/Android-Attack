#include "MapBright.h"
#include "../Config/Config.h"
#include <API/Il2CppApi.h>
#include <Tools/Tools.h>
#define LOGGER_TAG "ATTACK_LienQuan"
#include <Includes/Logger.h>
#include <Includes/obfuscate.h>

namespace lienquan {
namespace MapBright {

namespace {

using SetVisibleFn = bool (*)(Object *, int, bool, bool);

SetVisibleFn old_SetVisible = nullptr;

int getHostCamp() {
    Class *kf = FindClass(OBF("Kyrios.KyriosFramework"));
    if (!kf) return -1;
    Il2CppMethod *getHost = kf->find_method(OBF("get_hostLogic"), 0);
    if (!getHost) return -1;
    Object *host = getHost->static_invoke<Object *>();
    if (!host) return -1;
    return host->invoke_method<int>(OBF("get_hostPlayerCamp"));
}

bool hook_SetVisible(Object *self, int camp, bool bVisible, bool forceSync) {
    if (gLQConfig.main.mapHack && !bVisible) {
        const int hostCamp = getHostCamp();
        if (hostCamp < 0 || camp == hostCamp) bVisible = true;
    }
    return old_SetVisible(self, camp, bVisible, forceSync);
}

} // namespace

void InstallHooks() {
    void *setVisible = (void *)GET_METHOD("Project.Plugins_d.dll", "NucleusDrive.Logic", "LVActorLinker", "SetVisible", 3);
    Tools::Hook(setVisible, (void *)hook_SetVisible, (void **)&old_SetVisible);
}

} // namespace MapBright
} // namespace lienquan
