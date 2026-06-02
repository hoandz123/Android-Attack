#include "FishingGameplay.h"
#include "Config/Config.h"

extern bool isGameLoading;
#include "SDK/ActorControl.h"
#include "SDK/CacheUser.h"
#include "SDK/SystemHelper.h"
#include "SDK/enum/eFishingState.h"
#include "SDK/enum/eFishingFailType.h"
#include "SDK/enum/eMissionRewardState.h"
#include "SDK/enum/eTableType.h"
#include "SDK/enum/Illustbook_type.h"
#include "SDK/enum/Item_Type.h"
#include <API/Il2CppApi.h>
#include <Tools/Tools.h>
#include <atomic>
#include <cstring>
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
long long g_lastBaitEquipMs = 0;
long long g_lastGuideMs = 0;
long long g_lastAutoCatchMs = 0;
long long g_lastMissionClaimMs = 0;
bool g_guideArrowOn = false;
std::atomic<int> g_statusHint{0};

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

Object *getTablesRoot() {
    Object *tableSys = SystemHelper::get_Table();
    if (!tableSys) return nullptr;
    return tableSys->get_field_object<Object *>(OBF("Tables"));
}

Object *getTableImpl(eTableType type) {
    Object *tables = getTablesRoot();
    if (!tables) return nullptr;
    Class *tablesCls = tables->get_class();
    if (!tablesCls || !tablesCls->find_method(OBF("get_Item"), 1)) return nullptr;
    int key = (int) type;
    return tables->invoke_method<Object *>(OBF("get_Item"), key);
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

unsigned int readActiveZoneId(Object *fishingSys) {
    if (!fishingSys) return 0;
    unsigned int zone = fishingSys->get_field_value<unsigned int>(OBF("CastingFishingZoneID"));
    if (zone == 0) zone = fishingSys->invoke_method<unsigned int>(OBF("get_CatchFishingZone"));
    return zone;
}

unsigned int resolveAreaActionId(unsigned int zoneId) {
    if (zoneId == 0) return 0;
    Object *areaImpl = getTableImpl(eTableType::FishingArea);
    if (!areaImpl) return 0;
    Class *cls = areaImpl->get_class();
    if (!cls || !cls->find_method(OBF("GetTableData"), 1)) return 0;
    Object *area = areaImpl->invoke_method<Object *>(OBF("GetTableData"), zoneId);
    if (!area) return 0;
    Class *areaCls = area->get_class();
    if (!areaCls || !areaCls->find_method(OBF("get_ActionId"), 0)) return 0;
    return area->invoke_method<unsigned int>(OBF("get_ActionId"));
}

unsigned int pickBaitFromConfigZone(unsigned int zoneId) {
    if (zoneId == 0) return 0;
    for (const auto &entry : gPLConfig.fishing.baitZonePrefs) {
        if (entry.first == zoneId && entry.second > 0) return entry.second;
    }
    return 0;
}

unsigned int pickBaitByEffect(unsigned int actionId) {
    if (actionId == 0) return 0;
    Object *baitImpl = getTableImpl(eTableType::FishingBait);
    if (!baitImpl) return 0;
    Class *implCls = baitImpl->get_class();
    if (!implCls || !implCls->find_method(OBF("get_List"), 0)) return 0;
    Object *list = baitImpl->invoke_method<Object *>(OBF("get_List"));
    if (!list) return 0;
    Class *listCls = list->get_class();
    if (!listCls || !listCls->find_method(OBF("get_Count"), 0) || !listCls->find_method(OBF("get_Item"), 1)) return 0;
    int count = list->invoke_method<int>(OBF("get_Count"));
    unsigned int bestId = 0;
    unsigned int bestOrder = 0;
    for (int i = 0; i < count; i++) {
        Object *row = list->invoke_method<Object *>(OBF("get_Item"), i);
        if (!row) continue;
        Class *rowCls = row->get_class();
        if (!rowCls || !rowCls->find_method(OBF("get_EffectId"), 0) || !rowCls->find_method(OBF("get_BaitItemId"), 0)) continue;
        unsigned int effect = row->invoke_method<unsigned int>(OBF("get_EffectId"));
        unsigned int group = row->invoke_method<unsigned int>(OBF("get_CheckActionGroup"));
        if (effect != actionId && group != actionId) continue;
        unsigned int baitItem = row->invoke_method<unsigned int>(OBF("get_BaitItemId"));
        if (baitItem == 0 || CacheUser::GetItemCount(baitItem, true) <= 0) continue;
        unsigned int order = row->invoke_method<unsigned int>(OBF("get_Order"));
        if (bestId == 0 || order >= bestOrder) {
            bestId = baitItem;
            bestOrder = order;
        }
    }
    return bestId;
}

unsigned int resolveSmartBaitItemId(Object *fishingSys) {
    unsigned int zoneId = readActiveZoneId(fishingSys);
    if (gPLConfig.fishing.guideTargetZoneId > 0 && zoneId == 0) zoneId = gPLConfig.fishing.guideTargetZoneId;
    if (gPLConfig.fishing.smartBaitByZone) {
        unsigned int fromCfg = pickBaitFromConfigZone(zoneId);
        if (fromCfg > 0) return fromCfg;
    }
    if (gPLConfig.fishing.smartBaitAutoEffect) {
        unsigned int actionId = resolveAreaActionId(zoneId);
        unsigned int fromFx = pickBaitByEffect(actionId);
        if (fromFx > 0) return fromFx;
    }
    return (unsigned int) gPLConfig.fishing.baitItemId;
}

bool equipBaitUid(Object *fishingSys, unsigned int baitItemId) {
    if (!fishingSys || baitItemId == 0) return false;
    long long uid = CacheUser::GetItemUid(baitItemId);
    if (uid == 0) return false;
    long long cur = fishingSys->invoke_method<long long>(OBF("get_FishingBaitUID"));
    if (cur == uid) return true;
    long long now = Tools::getSystemMilliseconds();
    if (g_lastBaitEquipMs > 0 && now - g_lastBaitEquipMs < 1200) return false;
    g_lastBaitEquipMs = now;
    fishingSys->invoke_method<void>(OBF("set_FishingBaitUID"), uid);
    return true;
}

void TryAutoEquipBait(Object *fishingSys) {
    if (!gPLConfig.fishing.autoEquipBait || !fishingSys) return;
    g_statusHint.store(0, std::memory_order_relaxed);
    bool smart = gPLConfig.fishing.smartBaitByZone || gPLConfig.fishing.smartBaitAutoEffect;
    unsigned int baitId = smart ? resolveSmartBaitItemId(fishingSys) : (unsigned int) gPLConfig.fishing.baitItemId;
    if (baitId == 0) return;
    if (CacheUser::GetItemCount(baitId, true) <= 0) {
        if (smart || gPLConfig.fishing.baitItemId > 0) g_statusHint.store(1, std::memory_order_relaxed);
        return;
    }
    equipBaitUid(fishingSys, baitId);
}

bool isZoneFailType(int failType) {
    if (failType == (int) eFishingFailType::NoFishingZone) return true;
    if (failType == (int) eFishingFailType::InvalidCasting) return true;
    if (failType == (int) eFishingFailType::NotWater) return true;
    if (failType == (int) eFishingFailType::WaterLow) return true;
    return false;
}

void disableGuideArrow() {
    if (!g_guideArrowOn) return;
    Object *mapSys = SystemHelper::get_Map();
    if (!mapSys) return;
    Class *cls = mapSys->get_class();
    if (!cls || !cls->find_method(OBF("DisableGuideArrow"), 2)) return;
    bool reset = true;
    mapSys->invoke_method<void>(OBF("DisableGuideArrow"), gPLConfig.fishing.guidePointId, reset);
    g_guideArrowOn = false;
}

void enableGuideArrowIfNeeded(int castFailStreak, int lastFailType) {
    if (!gPLConfig.fishing.guideRouting) return;
    if (gPLConfig.fishing.guidePointId <= 0) return;
    int need = gPLConfig.fishing.guideFailStreak;
    if (need < 2) need = 2;
    if (castFailStreak < need) return;
    if (!isZoneFailType(lastFailType)) return;
    long long now = Tools::getSystemMilliseconds();
    if (g_lastGuideMs > 0 && now - g_lastGuideMs < 15000) return;
    Object *mapSys = SystemHelper::get_Map();
    if (!mapSys) return;
    Class *cls = mapSys->get_class();
    if (!cls || !cls->find_method(OBF("EnableGuideArrow"), 5)) return;
    g_lastGuideMs = now;
    g_guideArrowOn = true;
    bool useGpsOff = true;
    bool useLookAt = true;
    bool useAutoGpsOff = true;
    mapSys->invoke_method<void>(OBF("EnableGuideArrow"), gPLConfig.fishing.guidePointId, useGpsOff, useLookAt, useAutoGpsOff, (Object *) nullptr);
}

bool tryCheckFishingPoint(Object *player) {
    if (!player) return false;
    Class *cls = player->get_class();
    if (!cls || !cls->find_method(OBF("CheckFishingPoint"), 0)) return false;
    return player->invoke_method<bool>(OBF("CheckFishingPoint"));
}

void tryAutoCatchNetPoll() {
    if (!gPLConfig.fishing.autoCatchNetCheck) return;
    long long now = Tools::getSystemMilliseconds();
    int iv = gPLConfig.fishing.autoCatchIntervalMs;
    if (iv < 5000) iv = 5000;
    if (g_lastAutoCatchMs > 0 && now - g_lastAutoCatchMs < iv) return;
    Class *fwCls = FindClass(OBF("FrameWork"));
    if (!fwCls || !fwCls->find_method(OBF("get_Notification"), 0)) return;
    Object *noti = fwCls->find_method(OBF("get_Notification"), 0)->static_invoke<Object *>();
    if (!noti) return;
    Class *notiCls = noti->get_class();
    if (!notiCls || !notiCls->find_method(OBF("CheckAutoCatchFishingNet"), 0)) return;
    g_lastAutoCatchMs = now;
    noti->invoke_method<void>(OBF("CheckAutoCatchFishingNet"));
}

void tryClaimDailyMissionReward() {
    if (!gPLConfig.fishing.autoDailyMissionReward) return;
    long long now = Tools::getSystemMilliseconds();
    int iv = gPLConfig.fishing.missionClaimIntervalMs;
    if (iv < 2000) iv = 2000;
    if (iv > 5000) iv = 5000;
    if (g_lastMissionClaimMs > 0 && now - g_lastMissionClaimMs < iv) return;
    Object *missionSys = SystemHelper::get_Mission();
    if (!missionSys) return;
    Class *mCls = missionSys->get_class();
    if (!mCls || !mCls->find_method(OBF("get_HasDailyMissionReward"), 0)) return;
    if (!missionSys->invoke_method<bool>(OBF("get_HasDailyMissionReward"))) return;
    if (!mCls->find_method(OBF("GetDailyMissionToRewardState"), 1)) return;
    int rewardState = (int) eMissionRewardState::Reward;
    Object *daily = missionSys->invoke_method<Object *>(OBF("GetDailyMissionToRewardState"), rewardState);
    if (!daily) return;
    Class *dCls = daily->get_class();
    if (!dCls || !dCls->find_method(OBF("get_DailyMissionId"), 0)) return;
    if (dCls->find_method(OBF("get_IsAd"), 0) && daily->invoke_method<bool>(OBF("get_IsAd"))) return;
    unsigned int missionId = daily->invoke_method<unsigned int>(OBF("get_DailyMissionId"));
    if (missionId == 0) return;
    Object *netNative = nullptr;
    IL2Cpp::Il2CppGetStaticFieldValue(OBF("NetworkSystem"), OBF("NetNative"), &netNative);
    if (!netNative) return;
    Class *netCls = netNative->get_class();
    if (!netCls || !netCls->find_method(OBF("SendToMissionReward"), 2)) return;
    g_lastMissionClaimMs = now;
    netNative->invoke_method<void>(OBF("SendToMissionReward"), missionId, (Object *) nullptr);
}

void TickAuxMechanics(Object *player, Object *fishingSys, int fishingState, bool isFishing) {
    (void) fishingState;
    if (!il2cpp_loaded.load() || isGameLoading) return;
    if (!isFishing) {
        tryAutoCatchNetPoll();
        tryClaimDailyMissionReward();
    }
    (void) player;
    (void) fishingSys;
}

void OnCastRecovered() {
    disableGuideArrow();
}

const char *GetStatusHint() {
    switch (g_statusHint.load(std::memory_order_relaxed)) {
        case 1: return OBF("Hết mồi");
        default: return nullptr;
    }
}

void OnCastFailed(int castFailStreak, int lastFailType) {
    enableGuideArrowIfNeeded(castFailStreak, lastFailType);
}

bool TryCheckFishingPoint(Object *player) {
    if (!gPLConfig.fishing.guideRouting || gPLConfig.fishing.guidePointId <= 0) return true;
    return tryCheckFishingPoint(player);
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

int shadowIndexFromAssetName(const char *name) {
    if (!name || !name[0]) return 0;
    if (strcmp(name, OBF("fish_s_shadow")) == 0) return 1;
    if (strcmp(name, OBF("fish_m_shadow")) == 0) return 2;
    if (strcmp(name, OBF("fish_l_shadow")) == 0) return 3;
    if (strcmp(name, OBF("fish_xl_shadow")) == 0) return 4;
    if (strcmp(name, OBF("fish_xxl_shadow")) == 0) return 5;
    if (strcmp(name, OBF("fish_xxxl_shadow")) == 0) return 6;
    if (strcmp(name, OBF("fish_4xl_shadow")) == 0) return 7;
    return 0;
}

unsigned int GetCachedCastDifficultyId() {
    return g_fishingDifficultyId;
}

bool QueryFishDifficulty(unsigned int sid, int *outShadowIndex, unsigned int *outDifficultyId) {
    if (sid == 0) return false;
    Object *impl = getTableImpl(eTableType::FishingDifficulty);
    if (!impl) return false;
    Class *cls = impl->get_class();
    if (!cls || !cls->find_method(OBF("GetTableData"), 1)) return false;
    Object *row = impl->invoke_method<Object *>(OBF("GetTableData"), sid);
    if (!row) return false;
    Class *rowCls = row->get_class();
    if (!rowCls) return false;
    if (outDifficultyId) {
        if (!rowCls->find_method(OBF("get_FishingDifficultyId"), 0)) return false;
        *outDifficultyId = row->invoke_method<unsigned int>(OBF("get_FishingDifficultyId"));
    }
    if (outShadowIndex) {
        *outShadowIndex = 0;
        if (!rowCls->find_method(OBF("get_AssetName"), 0)) return false;
        String *asset = row->invoke_method<String *>(OBF("get_AssetName"));
        if (!asset) return false;
        *outShadowIndex = shadowIndexFromAssetName(asset->to_string().c_str());
    }
    return true;
}

const char *ShadowLabelFromIndex(int index) {
    switch (index) {
        case 1: return OBF("S");
        case 2: return OBF("M");
        case 3: return OBF("L");
        case 4: return OBF("XL");
        case 5: return OBF("XXL");
        case 6: return OBF("XXXL");
        case 7: return OBF("4XL");
        default: return OBF("?");
    }
}

} // namespace FishingGameplay
