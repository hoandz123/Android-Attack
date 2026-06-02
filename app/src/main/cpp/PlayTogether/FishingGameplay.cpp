#include "FishingGameplay.h"
#include "AutoFishing.h"
#include "FishingCatalog.h"
#include "Config/Config.h"

extern bool isGameLoading;
#include "SDK/ActorControl.h"
#include "SDK/CacheUser.h"
#include "SDK/SystemHelper.h"
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

void (*old_ReceiveFishingCatch)(Object *self, Object *rewardInfo) = nullptr;
void (*old_PID_FISHING_CASTING)(Object *self, Object *protocol) = nullptr;
bool g_hooksInstalled = false;
unsigned int g_fishingDifficultyId = 0;
bool g_earlyCatchReady = false;
bool g_earlyCatchSell = false;
long long g_lastBaitEquipMs = 0;

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

void cacheCastingProtocol(Object *protocol) {
    if (!protocol) return;
    g_fishingDifficultyId = protocol->invoke_method<unsigned int>(OBF("get_FishingDifficultyID"));
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

void hook_ReceiveFishingCatch(Object *self, Object *rewardInfo) {
    if (old_ReceiveFishingCatch) old_ReceiveFishingCatch(self, rewardInfo);
    if (!il2cpp_loaded.load() || isGameLoading || !rewardInfo) return;
    unsigned int itemId = rewardInfo->invoke_method<unsigned int>(OBF("get_CatchItemID"));
    unsigned int extraId = rewardInfo->invoke_method<unsigned int>(OBF("get_ExtraCatchItemID"));
    evaluateEarlyCatch(self, itemId, extraId);
    unsigned int levelId = AutoFishing::GetCurrentFishLevel();
    if (levelId == 0) levelId = GetCachedCastDifficultyId();
    bool learnedNew = false;
    if (RecordLearnedLevelFish(levelId, itemId)) learnedNew = true;
    if (extraId > 0 && RecordLearnedLevelFish(levelId, extraId)) learnedNew = true;
    if (learnedNew) {
        SaveConfig();
        FishingCatalog::RequestRebuild();
    }
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
    Il2CppMethod *mCatch = fishSys->find_method(OBF("ReceiveFishingCatch"), 1);
    if (mCatch && mCatch->methodPointer) Tools::Hook(mCatch->methodPointer, (void *) hook_ReceiveFishingCatch, (void **) &old_ReceiveFishingCatch);
    if (netNative) {
        Il2CppMethod *mCast = netNative->find_method(OBF("PID_FISHING_CASTING"), 1);
        if (mCast && mCast->methodPointer) Tools::Hook(mCast->methodPointer, (void *) hook_PID_FISHING_CASTING, (void **) &old_PID_FISHING_CASTING);
    }
    g_hooksInstalled = true;
    LOGI(OBF("FishingGameplay: response hooks installed"));
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
    bool smart = gPLConfig.fishing.smartBaitByZone || gPLConfig.fishing.smartBaitAutoEffect;
    unsigned int baitId = smart ? resolveSmartBaitItemId(fishingSys) : (unsigned int) gPLConfig.fishing.baitItemId;
    if (baitId == 0) return;
    if (CacheUser::GetItemCount(baitId, true) <= 0) return;
    equipBaitUid(fishingSys, baitId);
}

bool ShouldKeepCatch(unsigned int itemId, int grade) {
    if (itemId == 0) return true;
    if (CacheUser::IsItemLocked(itemId)) return true;
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

bool GetEarlyCatchSellDecision(bool *outSell) {
    if (!g_earlyCatchReady || !outSell) return false;
    *outSell = g_earlyCatchSell;
    return true;
}

void ClearEarlyCatchDecision() {
    g_earlyCatchReady = false;
    g_earlyCatchSell = false;
}

int ShadowIndexFromAssetName(const char *name) {
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

bool RecordLearnedLevelFish(unsigned int levelId, unsigned int itemId) {
    if (levelId == 0 || itemId == 0) return false;
    auto &entries = gPLConfig.fishing.learnedLevelFish;
    for (auto &e : entries) {
        if (e.first != levelId) continue;
        for (unsigned int id : e.second) {
            if (id == itemId) return false;
        }
        e.second.push_back(itemId);
        return true;
    }
    entries.emplace_back(levelId, std::vector<unsigned int>{itemId});
    return true;
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
        *outShadowIndex = ShadowIndexFromAssetName(asset->to_string().c_str());
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
