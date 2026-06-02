#include "AutoFishing.h"
#include "FishingGameplay.h"
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

constexpr int kTickIntervalMs = 400;
constexpr int kActionIntervalMs = 500;
constexpr int kRestartDelayMs = 1500;

struct ResolvedMethods {
    bool ready = false;
    bool ok = false;
    bool hasCloseReward = false;
    bool hasSellReward = false;
    bool hasCatchGrade = false;
    bool hasCatchItemId = false;
    bool hasFishLevel = false;
    bool hasFishLeave = false;
    bool hasFishingMiss = false;
};

ResolvedMethods g_methods;
eFishingState g_lastState = eFishingState::None;
long long g_lastActionMs = 0;
long long g_lastCastAttemptMs = 0;
long long g_lastRewardMs = 0;
long long g_stateSinceMs = 0;
unsigned int g_castingZoneId = 0;
unsigned int g_catchZoneId = 0;
long long g_baitUid = 0;
unsigned int g_currentFishLevel = 0;
int g_currentShadowIndex = 0;
unsigned int g_currentDifficultyId = 0;
bool g_skipFishThisCycle = false;
unsigned int g_skipCycleLevel = 0;
long long g_lastSkipMs = 0;
bool g_filterTableWarned = false;

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
    g_methods.hasFishLevel = baseCls->find_method(OBF("get_FishLevel"), 0);
    g_methods.hasFishLeave = baseCls->find_method(OBF("FishLeave"), 0);
    g_methods.hasFishingMiss = baseCls->find_method(OBF("FishingMiss"), 0);
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
    if (g_lastActionMs > 0 && now - g_lastActionMs < kActionIntervalMs) return false;
    g_lastActionMs = now;
    return true;
}

int readCatchGrade(Object *fishingSys) {
    if (!fishingSys || !g_methods.hasCatchGrade) return 0;
    return (int) fishingSys->invoke_method<int>(OBF("get_CatchItemGrade"));
}

unsigned int readCatchItemId(Object *fishingSys) {
    if (!fishingSys || !g_methods.hasCatchItemId) return 0;
    return fishingSys->invoke_method<unsigned int>(OBF("get_CatchItemID"));
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
    unsigned int itemId = readCatchItemId(fishingSys);
    bool sellTrash = gPLConfig.fishing.autoSellTrash && g_methods.hasSellReward && grade > 0 && grade <= gPLConfig.fishing.maxSellGrade;
    if (gPLConfig.fishing.smartKeepSell) {
        if (FishingGameplay::ShouldKeepCatch(itemId, grade)) sellTrash = false;
        else if (grade > 0 && grade <= gPLConfig.fishing.maxSellGrade) sellTrash = true;
    }
    bool earlySell = false;
    if (FishingGameplay::GetEarlyCatchSellDecision(&earlySell)) sellTrash = earlySell;
    if (sellTrash) {
        if (!canActNow()) return;
        g_lastRewardMs = now;
        dlg->invoke_method<void>(OBF("OnClick_Selling"));
        FishingGameplay::ClearEarlyCatchDecision();
        return;
    }
    if (!gPLConfig.fishing.autoCloseReward || !g_methods.hasCloseReward) return;
    if (!canActNow()) return;
    g_lastRewardMs = now;
    dlg->invoke_method<void>(OBF("OnClick_ButtonClose"));
    FishingGameplay::ClearEarlyCatchDecision();
}

bool isFilterEnabled() {
    return gPLConfig.fishing.filterByShadow || gPLConfig.fishing.filterByLevel;
}

void resetSkipCycle() {
    g_skipFishThisCycle = false;
    g_skipCycleLevel = 0;
}

bool shouldKeepLevel(unsigned int levelId) {
    if (!gPLConfig.fishing.filterByLevel) return true;
    if (!gPLConfig.fishing.keepLevelIds.empty()) {
        for (unsigned int id : gPLConfig.fishing.keepLevelIds) {
            if (id == levelId) return true;
        }
        return false;
    }
    int minL = gPLConfig.fishing.levelMin;
    int maxL = gPLConfig.fishing.levelMax;
    if (minL <= 0 && maxL <= 0) return true;
    if (minL > 0 && (int) levelId < minL) return false;
    if (maxL > 0 && (int) levelId > maxL) return false;
    return true;
}

bool shouldKeepShadow(int shadowIndex) {
    if (!gPLConfig.fishing.filterByShadow) return true;
    if (shadowIndex < 1 || shadowIndex > 7) return true;
    return gPLConfig.fishing.keepShadow[shadowIndex - 1];
}

void updateCurrentFishInfo(Object *player) {
    g_currentFishLevel = 0;
    g_currentShadowIndex = 0;
    g_currentDifficultyId = 0;
    if (!player || !g_methods.hasFishLevel) return;
    unsigned int level = player->invoke_method<unsigned int>(OBF("get_FishLevel"));
    if (level == 0) level = FishingGameplay::GetCachedCastDifficultyId();
    if (level == 0) return;
    g_currentFishLevel = level;
    int shadowIdx = 0;
    unsigned int diffId = 0;
    if (!FishingGameplay::QueryFishDifficulty(level, &shadowIdx, &diffId)) {
        if (isFilterEnabled() && !g_filterTableWarned) {
            g_filterTableWarned = true;
            LOGE(OBF("updateCurrentFishInfo: không resolve FishingDifficulty table"));
        }
        return;
    }
    g_currentShadowIndex = shadowIdx;
    g_currentDifficultyId = diffId;
}

bool shouldKeepCurrentFish() {
    if (!isFilterEnabled()) return true;
    if (g_currentFishLevel == 0) return true;
    if (!shouldKeepLevel(g_currentFishLevel)) return false;
    if (!shouldKeepShadow(g_currentShadowIndex)) return false;
    return true;
}

bool isShadowFilterBlocking() {
    return isFilterEnabled() && g_skipFishThisCycle && !shouldKeepCurrentFish();
}

void trySkipUnwantedFish(Object *player, eFishingState state) {
    if (!isFilterEnabled() || !player) return;
    if (state != eFishingState::Idle && state != eFishingState::Search && state != eFishingState::Hit && state != eFishingState::Fighting) return;
    updateCurrentFishInfo(player);
    if (shouldKeepCurrentFish()) return;
    if (g_skipFishThisCycle && g_skipCycleLevel == g_currentFishLevel) return;
    long long now = Tools::getSystemMilliseconds();
    if (g_lastSkipMs > 0 && now - g_lastSkipMs < 800) return;
    if (!canActNow()) return;
    g_lastSkipMs = now;
    g_skipFishThisCycle = true;
    g_skipCycleLevel = g_currentFishLevel;
    if ((state == eFishingState::Hit || state == eFishingState::Fighting) && g_methods.hasFishingMiss) player->invoke_method<void>(OBF("FishingMiss"));
    else if (g_methods.hasFishLeave) player->invoke_method<void>(OBF("FishLeave"));
    LOGD(OBF("Bỏ cá: lv=%u bóng=%s"), g_currentFishLevel, FishingGameplay::ShadowLabelFromIndex(g_currentShadowIndex));
}

void refreshTelemetry(Object *player, Object *fishingSys, eFishingState state) {
    if (fishingSys) {
        g_castingZoneId = fishingSys->get_field_value<unsigned int>(OBF("CastingFishingZoneID"));
        if (g_methods.hasCatchGrade) g_catchZoneId = fishingSys->invoke_method<unsigned int>(OBF("get_CatchFishingZone"));
        g_baitUid = fishingSys->invoke_method<long long>(OBF("get_FishingBaitUID"));
    }
    if (player && readIsFishing(player) && (state == eFishingState::Idle || state == eFishingState::Search || state == eFishingState::Hit || state == eFishingState::Fighting)) updateCurrentFishInfo(player);
}

void tryStartFishing(Object *player, Object *fishingSys) {
    if (!player) return;
    long long now = Tools::getSystemMilliseconds();
    if (g_lastCastAttemptMs > 0 && now - g_lastCastAttemptMs < kRestartDelayMs) return;
    if (!readHasPole(player)) return;
    FishingGameplay::TryAutoEquipBait(fishingSys);
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
    if (!player) return;
    if (isShadowFilterBlocking()) return;
    if (!canActNow()) return;
    player->invoke_method<void>(OBF("FishingHit"));
}

void tryLift(Object *player) {
    if (!player) return;
    if (!canActNow()) return;
    bool success = true;
    player->invoke_method<void>(OBF("Lift"), success);
}

void trackStateChange(eFishingState state, Object *player) {
    if (state == g_lastState) return;
    if (state == eFishingState::None || state == eFishingState::Casting || state == eFishingState::Fail || state == eFishingState::Miss || state == eFishingState::Catch || state == eFishingState::Finish || state == eFishingState::Boast || state == eFishingState::CastingFail) {
        resetSkipCycle();
        if (state == eFishingState::None || state == eFishingState::Casting) {
            g_currentFishLevel = 0;
            g_currentShadowIndex = 0;
            g_currentDifficultyId = 0;
        }
    }
    g_lastState = state;
    g_stateSinceMs = Tools::getSystemMilliseconds();
}

void tickState(Object *player, Object *fishingSys, eFishingState state) {
    if (state == eFishingState::Boast || state == eFishingState::Finish) {
        tryHandleRewardDialog(fishingSys);
        return;
    }
    switch (state) {
        case eFishingState::None:
            tryStartFishing(player, fishingSys);
            break;
        case eFishingState::Casting:
        case eFishingState::SearchResult:
        case eFishingState::Idle:
            trySkipUnwantedFish(player, state);
            break;
        case eFishingState::Catch:
            break;
        case eFishingState::Search:
            trySkipUnwantedFish(player, state);
            break;
        case eFishingState::Hit:
            trySkipUnwantedFish(player, state);
            if (!isShadowFilterBlocking()) tryFishingHit(player);
            break;
        case eFishingState::Fighting:
            tryLift(player);
            break;
        case eFishingState::Fail:
        case eFishingState::Miss:
        case eFishingState::CastingFail:
            if (Tools::getSystemMilliseconds() - g_stateSinceMs > kRestartDelayMs) tryStartFishing(player, fishingSys);
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
    FishingGameplay::InitHooks();
    RATE_LIMIT(kTickIntervalMs);
    Object *player = ActorControl::my_Player;
    Object *fishingSys = getFishingSystem();
    if (!fishingSys) return;
    eFishingState state = readState(player);
    trackStateChange(state, player);
    refreshTelemetry(player, fishingSys, state);
    if (getRewardDialog()) tryHandleRewardDialog(fishingSys);
    tickState(player, fishingSys, state);
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

unsigned int GetCurrentFishLevel() {
    return g_currentFishLevel;
}

int GetCurrentShadowIndex() {
    return g_currentShadowIndex;
}

unsigned int GetCurrentDifficultyId() {
    return g_currentDifficultyId;
}

}
