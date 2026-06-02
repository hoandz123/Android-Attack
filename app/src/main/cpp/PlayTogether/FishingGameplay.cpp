#include "FishingGameplay.h"
#include "Config/Config.h"

extern bool isGameLoading;
#include "SDK/ActorControl.h"
#include "SDK/CacheUser.h"
#include "SDK/enum/eFishingState.h"
#include <API/Il2CppApi.h>
#include <Tools/Tools.h>
#include <Includes/obfuscate.h>
#define LOG_TAG OBF("ATTACK_FishingGameplay")
#include <Includes/Logger.h>

namespace FishingGameplay {

namespace {

void (*old_ReceiveFishingTug)(Object *self, Object *tugInfo) = nullptr;
long long g_lastPerfectLiftMs = 0;
long long g_lastFastBiteMs = 0;
bool g_hooksInstalled = false;

float getUnityTime() {
    Class *timeCls = FindClass(OBF("UnityEngine.Time"));
    if (!timeCls) return 0.f;
    Il2CppMethod *m = timeCls->find_method(OBF("get_time"), 0);
    if (!m) return 0.f;
    return m->static_invoke<float>();
}

Object *getFishingFloat(Object *player) {
    if (!player) return nullptr;
    return player->get_field_object<Object *>(OBF("_fishingFloat"));
}

bool canPerfectLiftNow() {
    long long now = Tools::getSystemMilliseconds();
    int ms = gPLConfig.fishing.perfectLiftIntervalMs;
    if (ms < 120) ms = 120;
    if (g_lastPerfectLiftMs > 0 && now - g_lastPerfectLiftMs < ms) return false;
    g_lastPerfectLiftMs = now;
    return true;
}

void hook_ReceiveFishingTug(Object *self, Object *tugInfo) {
    if (old_ReceiveFishingTug) old_ReceiveFishingTug(self, tugInfo);
    if (!il2cpp_loaded.load() || isGameLoading) return;
    if (!gPLConfig.fishing.autoPerfectTug || !tugInfo) return;
    int hp = tugInfo->invoke_method<int>(OBF("get_Hp"));
    if (hp <= 0) return;
    Object *player = ActorControl::my_Player;
    if (!player) return;
    Object *flt = getFishingFloat(player);
    if (!flt) return;
    bool dragAlert = flt->invoke_method<bool>(OBF("get_IsDragAlert"));
    if (!dragAlert) return;
    if (!canPerfectLiftNow()) return;
    bool success = true;
    player->invoke_method<void>(OBF("Lift"), success);
}

} // namespace

void InitHooks() {
    if (g_hooksInstalled) return;
    if (!il2cpp_loaded.load()) return;
    Class *fishSys = FindClass(OBF("FishingSystem"));
    if (!fishSys) {
        LOGE(OBF("InitHooks: FishingSystem class missing"));
        return;
    }
    Il2CppMethod *m = fishSys->find_method(OBF("ReceiveFishingTug"), 1);
    if (!m || !m->methodPointer) {
        LOGE(OBF("InitHooks: ReceiveFishingTug missing"));
        return;
    }
    Tools::Hook(m->methodPointer, (void *) hook_ReceiveFishingTug, (void **) &old_ReceiveFishingTug);
    g_hooksInstalled = true;
    LOGI(OBF("FishingGameplay: hooked FishingSystem.ReceiveFishingTug"));
}

void TickPerfectTug(Object *player, Object *fishingSys, int fishingState) {
    (void) fishingSys;
    if (!gPLConfig.fishing.autoPerfectTug || !player) return;
    auto state = (eFishingState) fishingState;
    bool normalFight = state == eFishingState::Fighting;
    bool bigTug = state == eFishingState::BigFish_Tug || state == eFishingState::BigFish_Pumpin
                  || state == eFishingState::BigFish_Drag || state == eFishingState::BigFish_Fighting;
    if (!normalFight && !bigTug) return;
    Object *flt = getFishingFloat(player);
    if (!flt) return;
    bool liftNow = false;
    if (flt->invoke_method<bool>(OBF("get_IsDragAlert"))) {
        liftNow = true;
    } else if (normalFight) {
        float needT = flt->get_field_value<float>(OBF("_needPumpinT"));
        float startT = flt->get_field_value<float>(OBF("_startPumpinT"));
        if (needT > 0.05f) {
            float t = getUnityTime();
            if (t - startT >= needT * 0.92f) liftNow = true;
        }
    }
    if (!liftNow) return;
    if (!canPerfectLiftNow()) return;
    bool liftOk = true;
    player->invoke_method<void>(OBF("Lift"), liftOk);
}

void TryFastBite(Object *player, int fishingState) {
    if (!gPLConfig.fishing.fastBite || !player) return;
    if ((eFishingState) fishingState != eFishingState::Search) return;
    long long now = Tools::getSystemMilliseconds();
    if (g_lastFastBiteMs > 0 && now - g_lastFastBiteMs < 800) return;
    g_lastFastBiteMs = now;
    player->invoke_method<void>(OBF("FishingBite"));
}

void TryAutoEquipBait(Object *fishingSys) {
    if (!gPLConfig.fishing.autoEquipBait || !fishingSys) return;
    unsigned int baitId = (unsigned int) gPLConfig.fishing.baitItemId;
    if (baitId == 0) return;
    long long uid = CacheUser::GetItemUid(baitId);
    if (uid == 0) return;
    long long cur = fishingSys->invoke_method<long long>(OBF("get_FishingBaitUID"));
    if (cur == uid) return;
    fishingSys->invoke_method<void>(OBF("set_FishingBaitUID"), uid);
}

bool ShouldKeepCatch(unsigned int itemId, int grade) {
    if (itemId == 0) return true;
    if (gPLConfig.fishing.targetFishItemId > 0 && itemId == (unsigned int) gPLConfig.fishing.targetFishItemId) return true;
    if (grade >= gPLConfig.fishing.smartKeepMinGrade) return true;
    int owned = CacheUser::GetItemCount(itemId, true);
    return owned < gPLConfig.fishing.smartKeepMaxOwned;
}

} // namespace FishingGameplay
