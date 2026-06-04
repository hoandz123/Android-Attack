#include "Hook.h"
#include "Bypass/AntiCheat.h"
#include "EspRuntime.h"
#include "SkinUnlock.h"
#include "../Config/Config.h"
#include "SDK/KyriosFramework.h"
#include "SDK/ActorManager.h"
#include <API/Il2CppApi.h>
#include <Tools/Tools.h>
#define LOGGER_TAG "ATTACK_LienQuan"
#include <Includes/Logger.h>
#include <Includes/obfuscate.h>

namespace lienquan {
namespace Hook {

namespace {

bool (*old_SetVisible)(Object *, int, bool, bool) = nullptr;
void (*old_StartDownloadReq)(Object *, Object *) = nullptr;
void (*old_LGameActorMgrUpdateLogic)(Object *, int) = nullptr;
void (*old_ActorManagerUpdate)(Object *) = nullptr;

bool hook_SetVisible(Object *self, int camp, bool bVisible, bool forceSync) {
    if (!bVisible && gLQConfig.main.mapHack) {
        const int hostCamp = KyriosFramework::GetIsRunning() ? KyriosFramework::GetHostPlayerCamp() : -1;
        if (hostCamp < 0 || camp == hostCamp) bVisible = true;
    }
    return old_SetVisible(self, camp, bVisible, forceSync);
}

void hook_StartDownloadReq(Object *self, Object *req) {
    if (gLQConfig.main.blockDlcDownload) return;
    if (old_StartDownloadReq) old_StartDownloadReq(self, req);
}

void hook_LGameActorMgrUpdateLogic(Object *self, int delta) {
    if (old_LGameActorMgrUpdateLogic) old_LGameActorMgrUpdateLogic(self, delta);
    static bool inHook = false;
    if (inHook || !self) return;
    inHook = true;
    EspRuntime::OnGameUpdate(self);
    inHook = false;
}

void hook_ActorManagerUpdate(Object *self) {
    if (old_ActorManagerUpdate) old_ActorManagerUpdate(self);
    static bool inHook = false;
    if (inHook || !self) return;
    inHook = true;
    EspRuntime::OnActorManagerUpdate(self);
    inHook = false;
}

} // namespace

void init() {
    LoadConfig();
    AntiCheat::init();
    Tools::Hook((void *)GET_METHOD("Project.Plugins_d.dll", "NucleusDrive.Logic", "LVActorLinker", "SetVisible", 3), (void *)hook_SetVisible, (void **)&old_SetVisible);
    Tools::Hook((void *)GET_METHOD("ResCore.Runtime.dll", "ResCore.DLC", "DLCFileDownloadSession", "StartDownloadReq", 1), (void *)hook_StartDownloadReq, (void **)&old_StartDownloadReq);
    Tools::Hook((void *)GET_METHOD("Project.Plugins_d.dll", "NucleusDrive.Logic", "LGameActorMgr", "UpdateLogic", 1),
                (void *)hook_LGameActorMgrUpdateLogic, (void **)&old_LGameActorMgrUpdateLogic);
    Tools::Hook((void *)GET_METHOD("Project_d.dll", "Kyrios.Actor", "ActorManager", "Update", 0),
                (void *)hook_ActorManagerUpdate, (void **)&old_ActorManagerUpdate);

    SkinUnlock::init();
    LOGI(OBF("[LienQuan] Hook init done (ESP logic+Kyrios icons)"));
}

} // namespace Hook
} // namespace lienquan
