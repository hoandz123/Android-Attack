#include "AutoFishing.h"
#include "Config/Config.h"
#include "SDK/ActorControl.h"
#include "SDK/SystemHelper.h"
#include "SDK/enum/eFishingState.h"
#include "SDK/enum/eFishingFailType.h"
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
    bool hasSellReward = false;
    bool hasCatchGrade = false;
    bool hasCatchItemId = false;
    bool hasFloatPos = false;
    bool hasCatchFishId = false;
    bool hasFailTypeField = false;
    bool hasBigFishHp = false;
};

ResolvedMethods g_methods;
eFishingState g_lastState = eFishingState::None;
int g_fishCaught = 0;
int g_failCount = 0;
int g_missCount = 0;
int g_catchByGrade[6] = {};
long long g_lastActionMs = 0;
long long g_lastCastAttemptMs = 0;
long long g_lastRewardMs = 0;
long long g_stateSinceMs = 0;
long long g_sessionStartMs = 0;
bool g_pausedByRare = false;
bool g_rareAlert = false;
unsigned int g_lastCatchItemId = 0;
int g_lastCatchGrade = 0;
Vector3 g_floatPoint{};
bool g_hasFloatPoint = false;
unsigned int g_castingZoneId = 0;
unsigned int g_catchZoneId = 0;
long long g_baitUid = 0;
int g_lastFailType = 0;
bool g_fishingCountOver = false;
int g_bigFishHp = 0;
int g_bigFishHpMax = 0;
int g_castFailStreak = 0;

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
    Class *fishSysCls = FindClass(OBF("FishingSystem"));
    if (!playerCls || !baseCls) return false;
    bool hasBase = baseCls->find_method(OBF("get_FishingState"), 0) && baseCls->find_method(OBF("get_IsFishing"), 0) && baseCls->find_method(OBF("get_HasFishingPole"), 0);
    bool hasPlayer = playerCls->find_method(OBF("StartFishing"), 0) && playerCls->find_method(OBF("FishingHit"), 0) && playerCls->find_method(OBF("Lift"), 1);
    g_methods.hasCloseReward = dlgCls && dlgCls->find_method(OBF("OnClick_ButtonClose"), 0);
    g_methods.hasSellReward = dlgCls && dlgCls->find_method(OBF("OnClick_Selling"), 0);
    g_methods.hasCatchGrade = fishSysCls && fishSysCls->find_method(OBF("get_CatchItemGrade"), 0);
    g_methods.hasCatchItemId = fishSysCls && fishSysCls->find_method(OBF("get_CatchItemID"), 0);
    g_methods.hasFloatPos = baseCls->find_method(OBF("get_FishingFloatPos"), 0);
    g_methods.hasCatchFishId = baseCls->find_method(OBF("get_CatchFishID"), 0);
    g_methods.hasFailTypeField = playerCls != nullptr;
    g_methods.hasBigFishHp = fishSysCls && fishSysCls->find_method(OBF("get_BigFishHP"), 0) && fishSysCls->find_method(OBF("get_BigFishHP_Org"), 0);
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

bool stateDelayElapsed(int delayMs) {
    if (delayMs <= 0) return true;
    return Tools::getSystemMilliseconds() - g_stateSinceMs >= (long long) delayMs;
}

int readCatchGrade(Object *fishingSys) {
    if (!fishingSys || !g_methods.hasCatchGrade) return 0;
    return (int) fishingSys->invoke_method<int>(OBF("get_CatchItemGrade"));
}

unsigned int readCatchItemId(Object *fishingSys) {
    if (!fishingSys || !g_methods.hasCatchItemId) return 0;
    return fishingSys->invoke_method<unsigned int>(OBF("get_CatchItemID"));
}

void recordCatchStats(Object *player, Object *fishingSys) {
    int grade = readCatchGrade(fishingSys);
    unsigned int itemId = readCatchItemId(fishingSys);
    if (itemId == 0 && player && g_methods.hasCatchFishId) itemId = player->invoke_method<unsigned int>(OBF("get_CatchFishID"));
    g_lastCatchItemId = itemId;
    g_lastCatchGrade = grade;
    if (grade >= 1 && grade <= 5) g_catchByGrade[grade]++;
    int minRare = gPLConfig.fishing.minRareGrade;
    if (minRare < 1) minRare = 1;
    if (minRare > 5) minRare = 5;
    if (grade >= minRare) {
        g_rareAlert = true;
        if (gPLConfig.fishing.pauseOnRareCatch) g_pausedByRare = true;
    }
}

Object *getRewardDialog() {
    Object *dlg = nullptr;
    IL2Cpp::Il2CppGetStaticFieldValue(OBF("DialogFishingGetItem"), OBF("Self"), &dlg);
    return dlg;
}

void tryHandleRewardDialog(Object *fishingSys) {
    long long now = Tools::getSystemMilliseconds();
    if (g_lastRewardMs > 0 && now - g_lastRewardMs < 600) return;
    Object *dlg = getRewardDialog();
    if (!dlg) return;
    int grade = readCatchGrade(fishingSys);
    if (gPLConfig.fishing.autoSellTrash && g_methods.hasSellReward && grade > 0 && grade <= gPLConfig.fishing.maxSellGrade) {
        if (!canActNow()) return;
        g_lastRewardMs = now;
        dlg->invoke_method<void>(OBF("OnClick_Selling"));
        return;
    }
    if (!gPLConfig.fishing.autoCloseReward || !g_methods.hasCloseReward) return;
    if (!canActNow()) return;
    g_lastRewardMs = now;
    dlg->invoke_method<void>(OBF("OnClick_ButtonClose"));
}

int readFailType(Object *player) {
    if (!player || !g_methods.hasFailTypeField) return 0;
    return (int) player->get_field_value<int>(OBF("_fishingFailType"));
}

void refreshTelemetry(Object *player, Object *fishingSys, eFishingState state) {
    g_hasFloatPoint = false;
    g_bigFishHp = 0;
    g_bigFishHpMax = 0;
    g_fishingCountOver = false;
    if (player && g_methods.hasFloatPos && readIsFishing(player)) {
        g_floatPoint = player->invoke_method<Vector3>(OBF("get_FishingFloatPos"));
        g_hasFloatPoint = true;
    }
    if (fishingSys) {
        g_castingZoneId = fishingSys->get_field_value<unsigned int>(OBF("CastingFishingZoneID"));
        if (g_methods.hasCatchGrade) g_catchZoneId = fishingSys->invoke_method<unsigned int>(OBF("get_CatchFishingZone"));
        g_baitUid = fishingSys->invoke_method<long long>(OBF("get_FishingBaitUID"));
        g_fishingCountOver = fishingSys->invoke_method<bool>(OBF("get_IsFishingCountOver"));
        if (gPLConfig.fishing.handleBigFish && g_methods.hasBigFishHp && isBigFishState(state)) {
            g_bigFishHp = fishingSys->invoke_method<int>(OBF("get_BigFishHP"));
            g_bigFishHpMax = fishingSys->invoke_method<int>(OBF("get_BigFishHP_Org"));
        }
    }
}

void tryStartFishing(Object *player, Object *fishingSys) {
    if (!player) return;
    if (g_pausedByRare) return;
    long long now = Tools::getSystemMilliseconds();
    int delay = gPLConfig.fishing.restartDelayMs;
    if (delay < 800) delay = 800;
    if (gPLConfig.fishing.adaptiveCastBackoff && g_castFailStreak > 0) {
        int extra = g_castFailStreak * 450;
        if (extra > 4500) extra = 4500;
        delay += extra;
    }
    if (g_lastCastAttemptMs > 0 && now - g_lastCastAttemptMs < delay) return;
    if (!readHasPole(player)) return;
    if (readIsFishing(player)) return;
    Object *sys = fishingSys;
    if (sys) {
        bool countOver = sys->invoke_method<bool>(OBF("get_IsFishingCountOver"));
        if (countOver && gPLConfig.fishing.stopWhenCountOver) return;
        bool sysFishing = sys->get_field_value<bool>(OBF("IsFishing"));
        if (sysFishing && readState(player) == eFishingState::None) return;
    }
    if (!canActNow()) return;
    g_lastCastAttemptMs = now;
    bool started = player->invoke_method<bool>(OBF("StartFishing"));
    if (!started) LOGD(OBF("StartFishing trả về false"));
}

void tryFishingHit(Object *player) {
    if (!player || g_pausedByRare) return;
    if (!stateDelayElapsed(gPLConfig.fishing.hitDelayMs)) return;
    if (!canActNow()) return;
    player->invoke_method<void>(OBF("FishingHit"));
}

void tryLift(Object *player) {
    if (!player || g_pausedByRare) return;
    if (!stateDelayElapsed(gPLConfig.fishing.liftDelayMs)) return;
    if (!canActNow()) return;
    bool success = true;
    player->invoke_method<void>(OBF("Lift"), success);
}

void tryStunHit(Object *player) {
    if (!player || g_pausedByRare) return;
    if (!canActNow()) return;
    player->invoke_method<void>(OBF("FishingStunHit"));
}

void trackStateChange(eFishingState state, Object *player, Object *fishingSys) {
    if (state == g_lastState) return;
    if (state == eFishingState::CastingFail) {
        g_castFailStreak++;
        g_lastFailType = readFailType(player);
    }
    if (state == eFishingState::Search || state == eFishingState::Hit || state == eFishingState::Fighting) {
        g_castFailStreak = 0;
    }
    if (g_lastState == eFishingState::Fighting && state == eFishingState::Catch) {
        g_fishCaught++;
        recordCatchStats(player, fishingSys);
    }
    if (g_lastState == eFishingState::BigFish_Fighting && state == eFishingState::BigFish_Catch) {
        g_fishCaught++;
        recordCatchStats(player, fishingSys);
    }
    if (state == eFishingState::Fail || state == eFishingState::CastingFail) g_failCount++;
    if (state == eFishingState::Miss || state == eFishingState::BigFish_Miss) g_missCount++;
    g_lastState = state;
    g_stateSinceMs = Tools::getSystemMilliseconds();
}

void tickState(Object *player, Object *fishingSys, eFishingState state) {
    if (state == eFishingState::Boast || state == eFishingState::Finish) {
        tryHandleRewardDialog(fishingSys);
        if (gPLConfig.fishing.skipBoastDelay && state == eFishingState::Boast) {
            int ms = gPLConfig.fishing.boastSkipMs;
            if (ms < 200) ms = 200;
            if (stateDelayElapsed(ms)) tryHandleRewardDialog(fishingSys);
        }
        return;
    }
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
    if (g_sessionStartMs == 0) g_sessionStartMs = Tools::getSystemMilliseconds();
    RATE_LIMIT(gPLConfig.fishing.tickIntervalMs > 0 ? gPLConfig.fishing.tickIntervalMs : 400);
    Object *player = ActorControl::my_Player;
    Object *fishingSys = getFishingSystem();
    if (!fishingSys) return;
    eFishingState state = readState(player);
    trackStateChange(state, player, fishingSys);
    refreshTelemetry(player, fishingSys, state);
    if (getRewardDialog()) tryHandleRewardDialog(fishingSys);
    tickState(player, fishingSys, state);
}

eFishingState GetLastFishingState() {
    return g_lastState;
}

std::string GetStateLabel() {
    return std::string(stateName(g_lastState));
}

int GetFishCaughtCount() {
    return g_fishCaught;
}

int GetFailCount() {
    return g_failCount;
}

int GetMissCount() {
    return g_missCount;
}

int GetCatchCountByGrade(int grade) {
    if (grade < 0 || grade > 5) return 0;
    return g_catchByGrade[grade];
}

bool IsPausedByRare() {
    return g_pausedByRare;
}

bool HasRareAlert() {
    return g_rareAlert;
}

void ClearRareAlert() {
    g_rareAlert = false;
    g_pausedByRare = false;
}

unsigned int GetLastCatchItemId() {
    return g_lastCatchItemId;
}

int GetLastCatchGrade() {
    return g_lastCatchGrade;
}

unsigned int GetSessionElapsedSec() {
    if (g_sessionStartMs == 0) return 0;
    long long elapsed = Tools::getSystemMilliseconds() - g_sessionStartMs;
    if (elapsed < 0) elapsed = 0;
    return (unsigned int) (elapsed / 1000);
}

bool HasFloatPoint() {
    return g_hasFloatPoint;
}

Vector3 GetFloatPoint() {
    return g_floatPoint;
}

unsigned int GetCastingZoneId() {
    return g_castingZoneId;
}

unsigned int GetCatchZoneId() {
    return g_catchZoneId;
}

long long GetFishingBaitUid() {
    return g_baitUid;
}

int GetLastFailType() {
    return g_lastFailType;
}

bool IsFishingCountOver() {
    return g_fishingCountOver;
}

int GetBigFishHp() {
    return g_bigFishHp;
}

int GetBigFishHpMax() {
    return g_bigFishHpMax;
}

int GetCastFailStreak() {
    return g_castFailStreak;
}

int GetCatchesPerHour() {
    unsigned int sec = GetSessionElapsedSec();
    if (sec < 30) return 0;
    return (int) ((long long) g_fishCaught * 3600 / (long long) sec);
}

int GetSuccessRatePercent() {
    int total = g_fishCaught + g_failCount + g_missCount;
    if (total <= 0) return 0;
    return (int) ((long long) g_fishCaught * 100 / (long long) total);
}

void ResetSessionStats() {
    g_fishCaught = 0;
    g_failCount = 0;
    g_missCount = 0;
    for (int i = 0; i < 6; i++) g_catchByGrade[i] = 0;
    g_lastCatchItemId = 0;
    g_lastCatchGrade = 0;
    g_lastFailType = 0;
    g_castFailStreak = 0;
    g_pausedByRare = false;
    g_rareAlert = false;
    g_sessionStartMs = Tools::getSystemMilliseconds();
}

}
