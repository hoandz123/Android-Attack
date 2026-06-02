#include "FishingGameplay.h"
#include "Config/Config.h"

extern bool isGameLoading;
#include "SDK/ActorControl.h"
#include "SDK/CacheUser.h"
#include "SDK/SystemHelper.h"
#include "SDK/enum/eFishingState.h"
#include "SDK/enum/eFishingFailType.h"
#include "SDK/enum/eTableType.h"
#include "SDK/enum/Illustbook_type.h"
#include "SDK/enum/Item_Type.h"
#include <API/Il2CppApi.h>
#include <Tools/Tools.h>
#include <Includes/obfuscate.h>
#define LOG_TAG OBF("ATTACK_FishingGameplay")
#include <Includes/Logger.h>

namespace FishingGameplay {

namespace {

void (*old_ReceiveFishingTug)(Object *self, Object *tugInfo) = nullptr;
void (*old_ReceiveFishingHit)(Object *self, Object *hitResult) = nullptr;
void (*old_ReceiveFishingCatch)(Object *self, Object *rewardInfo) = nullptr;
void (*old_ReceiveFishingBegin)(Object *self, unsigned int difficultyLevel, bool castingSuccess, float duration, int failType) = nullptr;
void (*old_PID_FISHING_CASTING)(Object *self, Object *protocol) = nullptr;
long long g_lastPerfectLiftMs = 0;
long long g_lastFastBiteMs = 0;
long long g_lastStunHitMs = 0;
long long g_lastRaidEnterMs = 0;
long long g_nextFlowDeadlineMs = 0;
long long g_captchaDeadlineMs = 0;
long long g_lastPacingFailMs = 0;
int g_pacingPenaltyMs = 0;
int g_stunHitsThisPhase = 0;
bool g_hooksInstalled = false;
bool g_raidModeActive = false;
bool g_miniGameFlag = false;
unsigned int g_raidIdx = 0;
unsigned int g_castingKey = 0;
unsigned int g_fishingDifficultyId = 0;
bool g_earlyCatchReady = false;
bool g_earlyCatchSell = false;

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

Object *getItemTableImpl() {
    static Object *s_table = nullptr;
    static bool s_tried = false;
    if (s_tried) return s_table;
    s_tried = true;
    Object *tableSys = SystemHelper::get_Table();
    if (!tableSys) return nullptr;
    Object *tables = tableSys->get_field_object<Object *>(OBF("Tables"));
    if (!tables) return nullptr;
    Class *tablesCls = tables->get_class();
    if (!tablesCls || !tablesCls->find_method(OBF("get_Item"), 1)) return nullptr;
    int itemKey = (int) eTableType::Item;
    s_table = tables->invoke_method<Object *>(OBF("get_Item"), itemKey);
    return s_table;
}

Object *getTableItem(unsigned int itemId) {
    if (itemId == 0) return nullptr;
    Object *itemTable = getItemTableImpl();
    if (!itemTable) return nullptr;
    Class *cls = itemTable->get_class();
    if (!cls || !cls->find_method(OBF("GetTableData"), 1)) return nullptr;
    return itemTable->invoke_method<Object *>(OBF("GetTableData"), itemId);
}

int getItemSellValue(unsigned int itemId) {
    Object *item = getTableItem(itemId);
    if (!item) return -1;
    Class *cls = item->get_class();
    if (!cls || !cls->find_method(OBF("get_SellValue"), 0)) return -1;
    return item->invoke_method<int>(OBF("get_SellValue"));
}

bool getItemIsSellable(unsigned int itemId) {
    Object *item = getTableItem(itemId);
    if (!item) return false;
    Class *cls = item->get_class();
    if (!cls || !cls->find_method(OBF("get_IsSell"), 0)) return false;
    return item->invoke_method<bool>(OBF("get_IsSell"));
}

int getItemGradeFromTable(unsigned int itemId) {
    Object *itemTable = getItemTableImpl();
    if (!itemTable) return 0;
    Class *cls = itemTable->get_class();
    if (!cls || !cls->find_method(OBF("GetGrade"), 1)) return 0;
    return itemTable->invoke_method<int>(OBF("GetGrade"), itemId);
}

bool isInCodex(unsigned int itemId) {
    if (itemId == 0) return true;
    Object *book = nullptr;
    IL2Cpp::Il2CppGetStaticFieldValue(OBF("ContentSystem"), OBF("IllustBook"), &book);
    if (!book) return true;
    Class *cls = book->get_class();
    if (!cls || !cls->find_method(OBF("FindItemInIllustBook"), 2)) return true;
    int bookType = (int) Illustbook_type::FishBook;
    Object *found = book->invoke_method<Object *>(OBF("FindItemInIllustBook"), bookType, itemId);
    return found != nullptr;
}

bool isInventoryFishPressureHigh() {
    int limit = CacheUser::GetInventoryLimitFish();
    if (limit <= 0) return false;
    int count = CacheUser::GetItemTypeCount((int) Item_Type::Fish);
    return count >= limit - 2;
}

void notePacingFail() {
    g_lastPacingFailMs = Tools::getSystemMilliseconds();
    g_pacingPenaltyMs += 2200;
    if (g_pacingPenaltyMs > 12000) g_pacingPenaltyMs = 12000;
}

void cacheCastingProtocol(Object *protocol) {
    if (!protocol) return;
    Class *cls = protocol->get_class();
    if (!cls || !cls->find_method(OBF("get_NextFlowTimeSecond"), 0)) return;
    float nextFlowSec = protocol->invoke_method<float>(OBF("get_NextFlowTimeSecond"));
    long long captchaTick = protocol->invoke_method<long long>(OBF("get_CaptchaLimitTick"));
    long long now = Tools::getSystemMilliseconds();
    g_castingKey = protocol->invoke_method<unsigned int>(OBF("get_Key"));
    g_fishingDifficultyId = protocol->invoke_method<unsigned int>(OBF("get_FishingDifficultyID"));
    g_nextFlowDeadlineMs = now + (long long) (nextFlowSec * 1000.f);
    if (captchaTick > 1000000000000LL) g_captchaDeadlineMs = captchaTick;
    else if (captchaTick > 0) g_captchaDeadlineMs = now + captchaTick;
    else g_captchaDeadlineMs = 0;
}

void tryAutoRaidEnter(unsigned int raidIdx) {
    if (!gPLConfig.fishing.autoRaidEnter || raidIdx == 0) return;
    long long now = Tools::getSystemMilliseconds();
    if (g_lastRaidEnterMs > 0 && now - g_lastRaidEnterMs < 3000) return;
    Object *player = ActorControl::my_Player;
    if (!player) return;
    Object *netNative = nullptr;
    IL2Cpp::Il2CppGetStaticFieldValue(OBF("NetworkSystem"), OBF("NetNative"), &netNative);
    if (!netNative) return;
    Class *cls = netNative->get_class();
    if (!cls || !cls->find_method(OBF("SendToFishingRaidEnter"), 2)) return;
    Vector3 pos = player->invoke_method<Vector3>(OBF("get_FishingFloatPos"));
    g_lastRaidEnterMs = now;
    g_raidModeActive = true;
    netNative->invoke_method<void>(OBF("SendToFishingRaidEnter"), raidIdx, pos);
    LOGI(OBF("AutoRaidEnter: raidIdx=%u"), raidIdx);
}

bool canPerfectLiftNow() {
    long long now = Tools::getSystemMilliseconds();
    int ms = gPLConfig.fishing.perfectLiftIntervalMs;
    if (ms < 120) ms = 120;
    if (g_lastPerfectLiftMs > 0 && now - g_lastPerfectLiftMs < ms) return false;
    g_lastPerfectLiftMs = now;
    return true;
}

bool canStunHitNow() {
    long long now = Tools::getSystemMilliseconds();
    int ms = gPLConfig.fishing.stunHitIntervalMs;
    if (ms < 180) ms = 180;
    if (g_lastStunHitMs > 0 && now - g_lastStunHitMs < ms) return false;
    g_lastStunHitMs = now;
    return true;
}

void tryStunHit(Object *player) {
    if (!player) return;
    int cap = gPLConfig.fishing.maxStunHitsPerPhase;
    if (cap < 1) cap = 1;
    if (g_stunHitsThisPhase >= cap) return;
    if (!canStunHitNow()) return;
    g_stunHitsThisPhase++;
    player->invoke_method<void>(OBF("FishingStunHit"));
}

void tryPerfectLift(Object *player) {
    if (!player || !canPerfectLiftNow()) return;
    bool success = true;
    player->invoke_method<void>(OBF("Lift"), success);
}

void evaluateEarlyCatch(Object *fishingSys, unsigned int itemId, unsigned int extraItemId) {
    g_earlyCatchReady = false;
    g_earlyCatchSell = false;
    if (itemId == 0) return;
    int grade = fishingSys ? (int) fishingSys->invoke_method<int>(OBF("get_CatchItemGrade")) : getItemGradeFromTable(itemId);
    if (grade <= 0) grade = getItemGradeFromTable(itemId);
    bool keep = ShouldKeepCatch(itemId, grade);
    if (extraItemId > 0 && ShouldKeepCatch(extraItemId, getItemGradeFromTable(extraItemId))) keep = true;
    bool sellTrash = !keep && gPLConfig.fishing.autoSellTrash && grade > 0 && grade <= gPLConfig.fishing.maxSellGrade;
    if (sellTrash && !getItemIsSellable(itemId)) sellTrash = false;
    g_earlyCatchReady = true;
    g_earlyCatchSell = sellTrash;
}

void hook_PID_FISHING_CASTING(Object *self, Object *protocol) {
    if (old_PID_FISHING_CASTING) old_PID_FISHING_CASTING(self, protocol);
    if (!il2cpp_loaded.load() || isGameLoading || !protocol) return;
    cacheCastingProtocol(protocol);
}

void hook_ReceiveFishingBegin(Object *self, unsigned int difficultyLevel, bool castingSuccess, float duration, int failType) {
    if (old_ReceiveFishingBegin) old_ReceiveFishingBegin(self, difficultyLevel, castingSuccess, duration, failType);
    if (!il2cpp_loaded.load() || isGameLoading) return;
    (void) self;
    (void) difficultyLevel;
    if (failType == (int) eFishingFailType::InvalidKey || failType == (int) eFishingFailType::InvalidStep) notePacingFail();
    else if (castingSuccess) g_pacingPenaltyMs = 0;
    if (gPLConfig.fishing.adaptivePacing && duration > 0.05f) {
        long long now = Tools::getSystemMilliseconds();
        long long deadline = now + (long long) (duration * 1000.f);
        if (deadline > g_nextFlowDeadlineMs) g_nextFlowDeadlineMs = deadline;
    }
}

void hook_ReceiveFishingHit(Object *self, Object *hitResult) {
    if (old_ReceiveFishingHit) old_ReceiveFishingHit(self, hitResult);
    if (!il2cpp_loaded.load() || isGameLoading || !hitResult) return;
    g_miniGameFlag = hitResult->invoke_method<bool>(OBF("get_MiniGameFlag"));
    g_raidIdx = hitResult->invoke_method<unsigned int>(OBF("get_RaidIdx"));
    if (g_miniGameFlag && g_raidIdx > 0) tryAutoRaidEnter(g_raidIdx);
}

void hook_ReceiveFishingCatch(Object *self, Object *rewardInfo) {
    if (old_ReceiveFishingCatch) old_ReceiveFishingCatch(self, rewardInfo);
    if (!il2cpp_loaded.load() || isGameLoading || !rewardInfo) return;
    unsigned int itemId = rewardInfo->invoke_method<unsigned int>(OBF("get_CatchItemID"));
    unsigned int extraId = rewardInfo->invoke_method<unsigned int>(OBF("get_ExtraCatchItemID"));
    evaluateEarlyCatch(self, itemId, extraId);
}

void hook_ReceiveFishingTug(Object *self, Object *tugInfo) {
    if (old_ReceiveFishingTug) old_ReceiveFishingTug(self, tugInfo);
    if (!il2cpp_loaded.load() || isGameLoading || !tugInfo) return;
    int hp = tugInfo->invoke_method<int>(OBF("get_Hp"));
    if (hp <= 0) {
        g_stunHitsThisPhase = 0;
        return;
    }
    Object *player = ActorControl::my_Player;
    if (!player) return;
    bool nextStun = tugInfo->invoke_method<bool>(OBF("get_NextTugStunFlag"));
    if (gPLConfig.fishing.stunOrchestrator && nextStun && (gPLConfig.fishing.handleBigFish || g_raidModeActive)) {
        tryStunHit(player);
        return;
    }
    if (!gPLConfig.fishing.autoPerfectTug) return;
    Object *flt = getFishingFloat(player);
    if (!flt) return;
    bool dragAlert = flt->invoke_method<bool>(OBF("get_IsDragAlert"));
    if (!dragAlert) {
        float needT = flt->get_field_value<float>(OBF("_needPumpinT"));
        float startT = flt->get_field_value<float>(OBF("_startPumpinT"));
        if (needT > 0.05f) {
            float t = getUnityTime();
            if (t - startT < needT * 0.92f) return;
        } else {
            return;
        }
    }
    tryPerfectLift(player);
}

} // namespace

void InitHooks() {
    if (g_hooksInstalled) return;
    if (!il2cpp_loaded.load()) return;
    Class *fishSys = FindClass(OBF("FishingSystem"));
    Class *netNative = FindClass(OBF("NetNativeProtocol"));
    if (!fishSys) {
        LOGE(OBF("InitHooks: FishingSystem class missing"));
        return;
    }
    Il2CppMethod *mTug = fishSys->find_method(OBF("ReceiveFishingTug"), 1);
    if (mTug && mTug->methodPointer) Tools::Hook(mTug->methodPointer, (void *) hook_ReceiveFishingTug, (void **) &old_ReceiveFishingTug);
    Il2CppMethod *mHit = fishSys->find_method(OBF("ReceiveFishingHit"), 1);
    if (mHit && mHit->methodPointer) Tools::Hook(mHit->methodPointer, (void *) hook_ReceiveFishingHit, (void **) &old_ReceiveFishingHit);
    Il2CppMethod *mCatch = fishSys->find_method(OBF("ReceiveFishingCatch"), 1);
    if (mCatch && mCatch->methodPointer) Tools::Hook(mCatch->methodPointer, (void *) hook_ReceiveFishingCatch, (void **) &old_ReceiveFishingCatch);
    Il2CppMethod *mBegin = fishSys->find_method(OBF("ReceiveFishingBegin"), 4);
    if (mBegin && mBegin->methodPointer) Tools::Hook(mBegin->methodPointer, (void *) hook_ReceiveFishingBegin, (void **) &old_ReceiveFishingBegin);
    if (netNative) {
        Il2CppMethod *mCast = netNative->find_method(OBF("PID_FISHING_CASTING"), 1);
        if (mCast && mCast->methodPointer) Tools::Hook(mCast->methodPointer, (void *) hook_PID_FISHING_CASTING, (void **) &old_PID_FISHING_CASTING);
    }
    g_hooksInstalled = true;
    LOGI(OBF("FishingGameplay: response hooks installed"));
}

void TickPerfectTug(Object *player, Object *fishingSys, int fishingState) {
    (void) fishingSys;
    if (!gPLConfig.fishing.autoPerfectTug || !player) return;
    auto state = (eFishingState) fishingState;
    bool normalFight = state == eFishingState::Fighting;
    bool bigTug = state == eFishingState::BigFish_Tug || state == eFishingState::BigFish_Pumpin || state == eFishingState::BigFish_Drag || state == eFishingState::BigFish_Fighting;
    if (!normalFight && !bigTug) return;
    Object *flt = getFishingFloat(player);
    if (!flt) return;
    bool liftNow = false;
    if (flt->invoke_method<bool>(OBF("get_IsDragAlert"))) liftNow = true;
    else if (normalFight) {
        float needT = flt->get_field_value<float>(OBF("_needPumpinT"));
        float startT = flt->get_field_value<float>(OBF("_startPumpinT"));
        if (needT > 0.05f) {
            float t = getUnityTime();
            if (t - startT >= needT * 0.92f) liftNow = true;
        }
    }
    if (!liftNow) return;
    tryPerfectLift(player);
}

void TryFastBite(Object *player, int fishingState) {
    if (!gPLConfig.fishing.fastBite || !player) return;
    if ((eFishingState) fishingState != eFishingState::Search) return;
    if (!CanPaceAct()) return;
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
    if (CacheUser::IsItemLocked(itemId)) return true;
    if (gPLConfig.fishing.targetFishItemId > 0 && itemId == (unsigned int) gPLConfig.fishing.targetFishItemId) return true;
    if (grade >= gPLConfig.fishing.smartKeepMinGrade) return true;
    if (gPLConfig.fishing.keepCodexFish && !isInCodex(itemId)) return true;
    int owned = CacheUser::GetItemCount(itemId, true);
    if (owned < gPLConfig.fishing.smartKeepMaxOwned) return true;
    if (!gPLConfig.fishing.smartKeepSell) return true;
    if (!getItemIsSellable(itemId)) return true;
    int sellVal = getItemSellValue(itemId);
    if (sellVal < 0) return true;
    if (isInventoryFishPressureHigh()) return sellVal >= gPLConfig.fishing.minSellValue;
    return sellVal >= gPLConfig.fishing.minSellValue;
}

bool CanPaceAct() {
    if (!gPLConfig.fishing.adaptivePacing) return true;
    long long now = Tools::getSystemMilliseconds();
    if (g_pacingPenaltyMs > 0 && g_lastPacingFailMs > 0 && now - g_lastPacingFailMs < g_pacingPenaltyMs) return false;
    if (g_nextFlowDeadlineMs > 0 && now < g_nextFlowDeadlineMs) return false;
    if (g_captchaDeadlineMs > 0 && now < g_captchaDeadlineMs) return false;
    return true;
}

bool IsRaidModeActive() {
    return g_raidModeActive;
}

bool GetEarlyCatchSellDecision(bool *outSell) {
    if (!g_earlyCatchReady || !outSell) return false;
    *outSell = g_earlyCatchSell;
    return true;
}

void ClearEarlyCatchDecision() {
    g_earlyCatchReady = false;
    g_earlyCatchSell = false;
}

bool HasPendingRaid() {
    return g_miniGameFlag && g_raidIdx > 0;
}

unsigned int GetPendingRaidIdx() {
    return g_raidIdx;
}

} // namespace FishingGameplay
