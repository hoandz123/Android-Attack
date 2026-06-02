#include "AutoFishing.h"
#include "Config/Config.h"
#include "SDK/ActorControl.h"
#include "SDK/SystemHelper.h"
#include "SDK/enum/eFishingState.h"
#include <API/Il2CppApi.h>
#include <Tools/Tools.h>
#include <Includes/obfuscate.h>
#define LOG_TAG OBF("ATTACK_AutoFishing")
#include <Includes/Logger.h>

namespace AutoFishing {

namespace {

struct ResolvedMethods {
    bool ready = false;
    bool ok = false;
    bool hasCloseReward = false;
};

ResolvedMethods g_methods;
eFishingState g_lastState = eFishingState::None;
int g_fishCaught = 0;
long long g_lastActionMs = 0;
long long g_lastCastAttemptMs = 0;
long long g_stateSinceMs = 0;

bool isBigFishState(eFishingState state) {
    return (int) state >= (int) eFishingState::BigFish_RaidEnter;
}

const char *stateName(eFishingState state) {
    switch (state) {
        case eFishingState::None: return OBF("Nghỉ");
        case eFishingState::Casting: return OBF("Quăng cần");
        case eFishingState::Search: return OBF("Chờ cắn");
        case eFishingState::SearchResult: return OBF("Kết quả tìm");
        case eFishingState::Idle: return OBF("Chờ");
        case eFishingState::Hit: return OBF("Cắn câu");
        case eFishingState::Fighting: return OBF("Giật cá");
        case eFishingState::Catch: return OBF("Kéo lên");
        case eFishingState::Fail: return OBF("Thất bại");
        case eFishingState::Boast: return OBF("Khoe cá");
        case eFishingState::Finish: return OBF("Xong");
        case eFishingState::CastingFail: return OBF("Quăng lỗi");
        case eFishingState::Miss: return OBF("Trượt");
        case eFishingState::BigFish_RaidEnter: return OBF("Cá lớn vào");
        case eFishingState::BigFish_RaidSync: return OBF("Đồng bộ raid");
        case eFishingState::BigFish_Begin: return OBF("Cá lớn bắt đầu");
        case eFishingState::BigFish_Pumpin: return OBF("Kéo (lớn)");
        case eFishingState::BigFish_Drag: return OBF("Kéo mạnh");
        case eFishingState::BigFish_Tug: return OBF("Giật (lớn)");
        case eFishingState::BigFish_Fighting: return OBF("Chiến (lớn)");
        case eFishingState::BigFish_Catch: return OBF("Bắt (lớn)");
        case eFishingState::BigFish_Miss: return OBF("Trượt (lớn)");
        case eFishingState::BigFish_RaidFighting: return OBF("Raid đánh");
        case eFishingState::BigFish_StunBegin: return OBF("Choáng");
        case eFishingState::BigFish_Stun: return OBF("Choáng giật");
        case eFishingState::BigFish_StunRecovery: return OBF("Hồi choáng");
        default: return OBF("?");
    }
}

bool resolveMethods() {
    if (g_methods.ready) return g_methods.ok;
    g_methods.ready = true;
    Class *playerCls = FindClass(OBF("ActorDefaultControlPlayer"));
    Class *baseCls = FindClass(OBF("ActorDefaultControl"));
    Class *dlgCls = FindClass(OBF("DialogFishingGetItem"));
    if (!playerCls || !baseCls) return false;
    bool hasBase = baseCls->find_method(OBF("get_FishingState"), 0) && baseCls->find_method(OBF("get_IsFishing"), 0) && baseCls->find_method(OBF("get_HasFishingPole"), 0);
    bool hasPlayer = playerCls->find_method(OBF("StartFishing"), 0) && playerCls->find_method(OBF("FishingHit"), 0) && playerCls->find_method(OBF("Lift"), 1);
    g_methods.hasCloseReward = dlgCls && dlgCls->find_method(OBF("OnClick_ButtonClose"), 0);
    g_methods.ok = hasBase && hasPlayer;
    if (!g_methods.ok) LOGE(OBF("resolveMethods: thiếu method câu cá"));
    return g_methods.ok;
}

Object *getFishingSystem() {
    if (!il2cpp_loaded.load()) return nullptr;
    return SystemHelper::get_Fishing();
}

eFishingState readState(Object *player) {
    if (!player) return eFishingState::None;
    return player->invoke_method<eFishingState>(OBF("get_FishingState"));
}

bool readIsFishing(Object *player) {
    if (!player) return false;
    return player->invoke_method<bool>(OBF("get_IsFishing"));
}

bool readHasPole(Object *player) {
    if (!player) return false;
    return player->invoke_method<bool>(OBF("get_HasFishingPole"));
}

bool canActNow() {
    long long now = Tools::getSystemMilliseconds();
    int interval = gPLConfig.fishing.actionIntervalMs;
    if (interval < 200) interval = 200;
    if (g_lastActionMs > 0 && now - g_lastActionMs < interval) return false;
    g_lastActionMs = now;
    return true;
}

void tryCloseRewardDialog() {
    if (!gPLConfig.fishing.autoCloseReward || !g_methods.hasCloseReward) return;
    Object *dlg = nullptr;
    IL2Cpp::Il2CppGetStaticFieldValue(OBF("DialogFishingGetItem"), OBF("Self"), &dlg);
    if (!dlg) return;
    dlg->invoke_method<void>(OBF("OnClick_ButtonClose"));
}

void tryStartFishing(Object *player, Object *fishingSys) {
    if (!player) return;
    long long now = Tools::getSystemMilliseconds();
    int delay = gPLConfig.fishing.restartDelayMs;
    if (delay < 800) delay = 800;
    if (g_lastCastAttemptMs > 0 && now - g_lastCastAttemptMs < delay) return;
    if (!readHasPole(player)) return;
    if (readIsFishing(player)) return;
    Object *sys = fishingSys;
    if (sys) {
        bool countOver = sys->invoke_method<bool>(OBF("get_IsFishingCountOver"));
        if (countOver) return;
        bool sysFishing = sys->get_field_value<bool>(OBF("IsFishing"));
        if (sysFishing && readState(player) == eFishingState::None) return;
    }
    if (!canActNow()) return;
    g_lastCastAttemptMs = now;
    bool started = player->invoke_method<bool>(OBF("StartFishing"));
    if (!started) LOGD(OBF("StartFishing trả về false"));
}

void tryFishingHit(Object *player) {
    if (!player) return;
    if (!canActNow()) return;
    player->invoke_method<void>(OBF("FishingHit"));
}

void tryLift(Object *player) {
    if (!player) return;
    if (!canActNow()) return;
    bool success = true;
    player->invoke_method<void>(OBF("Lift"), success);
}

void tryStunHit(Object *player) {
    if (!player) return;
    if (!canActNow()) return;
    player->invoke_method<void>(OBF("FishingStunHit"));
}

void trackStateChange(eFishingState state) {
    if (state == g_lastState) return;
    if (g_lastState == eFishingState::Fighting && state == eFishingState::Catch) g_fishCaught++;
    if (g_lastState == eFishingState::BigFish_Fighting && state == eFishingState::BigFish_Catch) g_fishCaught++;
    g_lastState = state;
    g_stateSinceMs = Tools::getSystemMilliseconds();
}

void tickState(Object *player, Object *fishingSys, eFishingState state) {
    if (isBigFishState(state)) {
        if (!gPLConfig.fishing.handleBigFish) return;
        if (state == eFishingState::BigFish_Stun || state == eFishingState::BigFish_StunBegin) tryStunHit(player);
        else if (state == eFishingState::BigFish_Tug || state == eFishingState::BigFish_Fighting || state == eFishingState::BigFish_Pumpin || state == eFishingState::BigFish_Drag) tryLift(player);
        return;
    }
    switch (state) {
        case eFishingState::None:
            tryStartFishing(player, fishingSys);
            break;
        case eFishingState::Casting:
        case eFishingState::Search:
        case eFishingState::SearchResult:
        case eFishingState::Idle:
        case eFishingState::Catch:
        case eFishingState::Boast:
        case eFishingState::Finish:
            break;
        case eFishingState::Hit:
            tryFishingHit(player);
            break;
        case eFishingState::Fighting:
            tryLift(player);
            break;
        case eFishingState::Fail:
        case eFishingState::Miss:
        case eFishingState::CastingFail:
            if (Tools::getSystemMilliseconds() - g_stateSinceMs > (long long) gPLConfig.fishing.restartDelayMs) tryStartFishing(player, fishingSys);
            break;
        default:
            break;
    }
}

}

void Update() {
    if (!gPLConfig.fishing.enabled) return;
    if (!il2cpp_loaded.load()) return;
    if (isGameLoading) return;
    if (!ActorControl::my_Player || !ActorControl::my_Motor || !ActorControl::my_Unit) return;
    if (!resolveMethods()) return;
    RATE_LIMIT(gPLConfig.fishing.tickIntervalMs > 0 ? gPLConfig.fishing.tickIntervalMs : 400);
    Object *player = ActorControl::my_Player;
    Object *fishingSys = getFishingSystem();
    if (!fishingSys) return;
    tryCloseRewardDialog();
    eFishingState state = readState(player);
    trackStateChange(state);
    tickState(player, fishingSys, state);
}

std::string GetStateLabel() {
    return std::string(stateName(g_lastState));
}

int GetFishCaughtCount() {
    return g_fishCaught;
}

}
