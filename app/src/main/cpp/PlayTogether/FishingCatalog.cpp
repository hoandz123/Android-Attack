#include "FishingCatalog.h"
#include "AutoFishing.h"
#include "SDK/CacheUser.h"
#include "SDK/SystemHelper.h"
#include "SDK/enum/eTableType.h"
#include "SDK/enum/Illustbook_type.h"
#include "SDK/enum/Fish_Type.h"
#include "SDK/enum/Item_Type.h"
#include <API/Il2CppApi.h>
#include <API/Il2cpp_Struct.h>
#include <Tools/Tools.h>
#include <cstdarg>
#include <cstring>
#include <Includes/obfuscate.h>
#define LOG_TAG OBF("ATTACK_FishingCatalog")
#include <Includes/Logger.h>

extern bool isGameLoading;

namespace FishingCatalog {

namespace {

std::atomic<bool> g_ready{false};
std::atomic<int> g_readIndex{0};
std::atomic<bool> g_forceRebuild{false};
std::atomic<bool> g_pickerOpen{false};
Snapshot g_buffers[2]{};
long long g_lastRebuildMs = 0;
int g_lastMapId = -1;
bool g_fishSessionBuilt = false;

void copyLabelFmt(char *dst, size_t cap, const char *fmt, ...) {
    if (!dst || cap == 0 || !fmt) return;
    va_list args;
    va_start(args, fmt);
    vsnprintf(dst, cap, fmt, args);
    va_end(args);
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
    return getTableImpl(eTableType::Item);
}

std::string resolveMessage(unsigned int msgId) {
    if (msgId == 0) return {};
    Class *msgCls = FindClass(OBF("TableMessagesImpl"));
    if (!msgCls || !msgCls->find_method(OBF("GetMessageDx"), 1)) return {};
    Il2CppMethod *m = msgCls->find_method(OBF("GetMessageDx"), 1);
    if (!m) return {};
    String *s = m->static_invoke<String *>(msgId);
    if (!s) return {};
    return s->to_string();
}

std::string resolveItemName(unsigned int itemId) {
    if (itemId == 0) return {};
    Object *itemTable = getItemTableImpl();
    if (!itemTable) return {};
    Class *cls = itemTable->get_class();
    if (!cls || !cls->find_method(OBF("GetNameID"), 1)) return {};
    unsigned int nameId = itemTable->invoke_method<unsigned int>(OBF("GetNameID"), itemId);
    std::string name = resolveMessage(nameId);
    if (!name.empty()) return name;
    char buf[48];
    snprintf(buf, sizeof(buf), OBF("Item %u"), itemId);
    return std::string(buf);
}

int resolveItemGrade(unsigned int itemId) {
    Object *itemTable = getItemTableImpl();
    if (!itemTable) return 0;
    Class *cls = itemTable->get_class();
    if (!cls || !cls->find_method(OBF("GetGrade"), 1)) return 0;
    return itemTable->invoke_method<int>(OBF("GetGrade"), itemId);
}

bool resolveItemIsFish(unsigned int itemId) {
    Object *itemTable = getItemTableImpl();
    if (!itemTable) return false;
    Class *cls = itemTable->get_class();
    if (!cls || !cls->find_method(OBF("GetItemType"), 1)) return false;
    int type = (int) itemTable->invoke_method<int>(OBF("GetItemType"), itemId);
    return type == (int) Item_Type::Fish;
}

bool resolveInCodex(unsigned int itemId) {
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

int readActiveGuidePoint() {
    Object *mapSys = SystemHelper::get_Map();
    if (!mapSys) return 0;
    Class *cls = mapSys->get_class();
    if (!cls || !cls->find_method(OBF("get_CurrentShowGuidePoint"), 0)) return 0;
    return mapSys->invoke_method<int>(OBF("get_CurrentShowGuidePoint"));
}

Object *getCurrentMap() {
    Object *mapSys = SystemHelper::get_Map();
    if (mapSys) {
        Object *cur = mapSys->get_field_object<Object *>(OBF("m_CurrentMap"));
        if (cur) return cur;
    }
    Class *fwCls = FindClass(OBF("FrameWork"));
    if (!fwCls || !fwCls->find_method(OBF("get_CurrentMap"), 0)) return nullptr;
    return fwCls->find_method(OBF("get_CurrentMap"), 0)->static_invoke<Object *>();
}

bool mapGuideIdListed(const int *ids, int count, int guidePointId) {
    for (int i = 0; i < count; i++) {
        if (ids[i] == guidePointId) return true;
    }
    return false;
}

int collectMapGuidePointIds(int *outIds, int maxCount) {
    if (!outIds || maxCount <= 0) return 0;
    static bool s_mapGuideSkipLogged = false;
    Object *map = getCurrentMap();
    if (!map) {
        if (!s_mapGuideSkipLogged) {
            LOGW(OBF("FishingCatalog: không đọc được map hiện tại — bỏ qua mapGuidePoints"));
            s_mapGuideSkipLogged = true;
        }
        return 0;
    }
    Class *mapCls = map->get_class();
    if (!mapCls) return 0;
    Object *rawList = map->get_field_object<Object *>(OBF("mapGuidePoints"));
    if (!rawList) {
        if (!s_mapGuideSkipLogged) {
            LOGW(OBF("FishingCatalog: mapGuidePoints null — chỉ dùng AutoCatchArea"));
            s_mapGuideSkipLogged = true;
        }
        return 0;
    }
    auto *list = (List<Object *> *)rawList;
    int count = list->get_Count();
    if (count <= 0) return 0;
    int added = 0;
    for (int i = 0; i < count && added < maxCount; i++) {
        Object *pt = list->get_item(i);
        if (!pt) continue;
        Class *ptCls = pt->get_class();
        if (!ptCls) continue;
        unsigned int gpId = pt->get_field_value<unsigned int>(OBF("guidePointID"));
        if (gpId == 0) continue;
        if (mapGuideIdListed(outIds, added, (int) gpId)) continue;
        outIds[added++] = (int) gpId;
    }
    return added;
}

void sortGuides(GuideEntry *entries, int count) {
    for (int i = 1; i < count; i++) {
        GuideEntry key = entries[i];
        int j = i - 1;
        while (j >= 0) {
            bool after = false;
            if (entries[j].onCurrentMap != key.onCurrentMap) after = entries[j].onCurrentMap < key.onCurrentMap;
            else if (entries[j].guidePointId > key.guidePointId) after = true;
            if (!after) break;
            entries[j + 1] = entries[j];
            j--;
        }
        entries[j + 1] = key;
    }
}

bool guideCatalogHasId(const Snapshot &snap, int guidePointId) {
    for (int i = 0; i < snap.guideCount; i++) {
        if (snap.guides[i].guidePointId == guidePointId) return true;
    }
    return false;
}

int readTableImplCount(Object *impl) {
    if (!impl) return -1;
    Class *cls = impl->get_class();
    if (!cls || !cls->find_method(OBF("get_Count"), 0)) return -2;
    return impl->invoke_method<int>(OBF("get_Count"));
}

int readTableDictCount(Object *impl) {
    if (!impl) return -1;
    Class *cls = impl->get_class();
    if (!cls || !cls->find_method(OBF("get_Container"), 0)) return -2;
    Object *rawDict = impl->invoke_method<Object *>(OBF("get_Container"));
    if (!rawDict) return -3;
    auto *dict = (Dictionary<uint32_t, Object *> *)rawDict;
    return dict->get_Count();
}

void logCatalogBuild(const char *tableName, Object *impl, int dictCount, int visited, int snapCount) {
    static long long s_lastLogMs = 0;
    long long now = Tools::getSystemMilliseconds();
    if (now - s_lastLogMs < 7000) return;
    s_lastLogMs = now;
    LOGI(OBF("FishingCatalog %s: impl=%p get_Count=%d dictCount=%d visited=%d snap=%d"), tableName, impl, readTableImplCount(impl), dictCount, visited, snapCount);
}

std::string resolveAutoCatchRowName(Object *row) {
    if (!row) return {};
    Class *rowCls = row->get_class();
    if (!rowCls) return {};
    unsigned int guideTextId = 0;
    unsigned int placeTextId = 0;
    unsigned int zoneTextId = 0;
    if (rowCls->find_method(OBF("get_GuideText"), 0)) guideTextId = row->invoke_method<unsigned int>(OBF("get_GuideText"));
    if (rowCls->find_method(OBF("get_AutoCatchPlaceText"), 0)) placeTextId = row->invoke_method<unsigned int>(OBF("get_AutoCatchPlaceText"));
    if (rowCls->find_method(OBF("get_AutoCatchZoneText"), 0)) zoneTextId = row->invoke_method<unsigned int>(OBF("get_AutoCatchZoneText"));
    std::string name = resolveMessage(guideTextId);
    if (name.empty()) name = resolveMessage(placeTextId);
    if (name.empty()) name = resolveMessage(zoneTextId);
    return name;
}

template<typename Fn>
void forEachDictValues(Object *impl, int cap, Fn fn) {
    if (!impl || cap <= 0) return;
    Class *cls = impl->get_class();
    if (!cls || !cls->find_method(OBF("get_Container"), 0)) return;
    Object *rawDict = impl->invoke_method<Object *>(OBF("get_Container"));
    if (!rawDict) return;
    auto *dict = (Dictionary<uint32_t, Object *> *)rawDict;
    int dictCount = dict->get_Count();
    if (dictCount <= 0) return;
    if (dictCount > cap * 4) dictCount = cap * 4;
    Object **values = dict->CollectValues();
    if (!values) return;
    int added = 0;
    for (int i = 0; i < dictCount && added < cap; i++) {
        Object *row = values[i];
        if (!row) continue;
        if (fn(row)) added++;
    }
    delete[] values;
}

std::string lookupAutoCatchGuideName(int guidePointId) {
    if (guidePointId <= 0) return {};
    Object *impl = getTableImpl(eTableType::AutoCatchArea);
    if (!impl) return {};
    std::string found;
    forEachDictValues(impl, kMaxGuides, [&](Object *row) -> bool {
        Class *rowCls = row->get_class();
        if (!rowCls || !rowCls->find_method(OBF("get_GuidePoint"), 0)) return false;
        int guidePoint = (int) row->invoke_method<unsigned int>(OBF("get_GuidePoint"));
        if (guidePoint != guidePointId) return false;
        found = resolveAutoCatchRowName(row);
        return false;
    });
    return found;
}

void buildBaits(Snapshot &snap, long long equippedUid) {
    snap.baitCount = 0;
    Object *cacheUser = CacheUser::get_Instance();
    if (!cacheUser) return;
    Class *cls = cacheUser->get_class();
    if (!cls || !cls->find_method(OBF("GetItemList"), 1)) return;
    int baitType = (int) Item_Type::BaitItem;
    Object *rawList = cacheUser->invoke_method<Object *>(OBF("GetItemList"), baitType);
    if (!rawList) return;
    auto *list = (List<Object *> *)rawList;
    int count = list->get_Count();
    if (count <= 0) return;
    if (count > kMaxBaits) count = kMaxBaits;
    for (int i = 0; i < count && snap.baitCount < kMaxBaits; i++) {
        Object *item = list->get_item(i);
        if (!item) continue;
        Class *itemCls = item->get_class();
        if (!itemCls) continue;
        if (!itemCls->find_method(OBF("get_ItemID"), 0)) continue;
        if (!itemCls->find_method(OBF("get_ItemUID"), 0)) continue;
        if (!itemCls->find_method(OBF("get_ItemCount"), 0)) continue;
        unsigned int itemId = item->invoke_method<unsigned int>(OBF("get_ItemID"));
        if (itemId == 0) continue;
        int itemCount = item->invoke_method<int>(OBF("get_ItemCount"));
        if (itemCount <= 0) continue;
        long long uid = item->invoke_method<long long>(OBF("get_ItemUID"));
        bool locked = false;
        if (itemCls->find_method(OBF("get_IsLock"), 0)) locked = item->invoke_method<bool>(OBF("get_IsLock"));
        int grade = resolveItemGrade(itemId);
        BaitEntry &e = snap.baits[snap.baitCount];
        e.itemId = itemId;
        e.uid = uid;
        e.count = itemCount;
        e.grade = grade;
        e.equipped = (equippedUid != 0 && uid == equippedUid);
        e.locked = locked;
        std::string name = resolveItemName(itemId);
        copyLabelFmt(e.label, kLabelLen, OBF("%s (%u) x%d%s%s"), name.c_str(), itemId, itemCount, e.equipped ? OBF(" [gắn]") : "", locked ? OBF(" [khóa]") : "");
        snap.baitCount++;
    }
}

void buildZones(Snapshot &snap, unsigned int castingZone, unsigned int catchZone) {
    snap.zoneCount = 0;
    Object *impl = getTableImpl(eTableType::FishingArea);
    int dictCount = readTableDictCount(impl);
    int visited = 0;
    if (!impl) {
        logCatalogBuild(OBF("FishingArea"), impl, dictCount, visited, snap.zoneCount);
        return;
    }
    forEachDictValues(impl, kMaxZones, [&](Object *row) -> bool {
        visited++;
        if (snap.zoneCount >= kMaxZones) return false;
        Class *rowCls = row->get_class();
        if (!rowCls || !rowCls->find_method(OBF("get_FishingZoneId"), 0)) return false;
        unsigned int zoneId = row->invoke_method<unsigned int>(OBF("get_FishingZoneId"));
        if (zoneId == 0) return false;
        unsigned int actionId = 0;
        unsigned int zoneTextId = 0;
        if (rowCls->find_method(OBF("get_ActionId"), 0)) actionId = row->invoke_method<unsigned int>(OBF("get_ActionId"));
        if (rowCls->find_method(OBF("get_FishingZoneText"), 0)) zoneTextId = row->invoke_method<unsigned int>(OBF("get_FishingZoneText"));
        std::string name = resolveMessage(zoneTextId);
        if (name.empty()) {
            char buf[32];
            snprintf(buf, sizeof(buf), OBF("Vùng %u"), zoneId);
            name = buf;
        }
        ZoneEntry &e = snap.zones[snap.zoneCount];
        e.zoneId = zoneId;
        e.actionId = actionId;
        e.isCurrent = (zoneId == castingZone || zoneId == catchZone);
        copyLabelFmt(e.label, kLabelLen, OBF("%s%s (%u)"), name.c_str(), e.isCurrent ? OBF(" [hiện tại]") : "", zoneId);
        snap.zoneCount++;
        return true;
    });
    logCatalogBuild(OBF("FishingArea"), impl, dictCount, visited, snap.zoneCount);
}

void buildGuides(Snapshot &snap, int activeGuidePoint) {
    snap.guideCount = 0;
    int mapGpIds[kMaxGuides]{};
    int mapGpCount = collectMapGuidePointIds(mapGpIds, kMaxGuides);
    Object *impl = getTableImpl(eTableType::AutoCatchArea);
    if (impl) {
        forEachDictValues(impl, kMaxGuides, [&](Object *row) -> bool {
            if (snap.guideCount >= kMaxGuides) return false;
            Class *rowCls = row->get_class();
            if (!rowCls || !rowCls->find_method(OBF("get_GuidePoint"), 0)) return false;
            int guidePoint = (int) row->invoke_method<unsigned int>(OBF("get_GuidePoint"));
            if (guidePoint <= 0) return false;
            unsigned int zoneId = 0;
            if (rowCls->find_method(OBF("get_AutoCatchZoneId"), 0)) zoneId = row->invoke_method<unsigned int>(OBF("get_AutoCatchZoneId"));
            std::string name = resolveAutoCatchRowName(row);
            if (name.empty()) {
                char buf[32];
                snprintf(buf, sizeof(buf), OBF("Guide %d"), guidePoint);
                name = buf;
            }
            GuideEntry &e = snap.guides[snap.guideCount];
            e.guidePointId = guidePoint;
            e.zoneId = zoneId;
            e.isActive = (activeGuidePoint > 0 && guidePoint == activeGuidePoint);
            e.onCurrentMap = mapGpCount > 0 && mapGuideIdListed(mapGpIds, mapGpCount, guidePoint);
            copyLabelFmt(e.label, kLabelLen, OBF("%s%s (ID %d)"), name.c_str(), e.isActive ? OBF(" [đang bật]") : "", guidePoint);
            snap.guideCount++;
            return true;
        });
    }
    for (int i = 0; i < mapGpCount && snap.guideCount < kMaxGuides; i++) {
        int gpId = mapGpIds[i];
        if (guideCatalogHasId(snap, gpId)) continue;
        GuideEntry &e = snap.guides[snap.guideCount];
        e.guidePointId = gpId;
        e.zoneId = 0;
        e.isActive = (activeGuidePoint > 0 && gpId == activeGuidePoint);
        e.onCurrentMap = true;
        std::string name = lookupAutoCatchGuideName(gpId);
        if (name.empty()) copyLabelFmt(e.label, kLabelLen, OBF("GP %d%s"), gpId, e.isActive ? OBF(" [đang bật]") : "");
        else copyLabelFmt(e.label, kLabelLen, OBF("%s%s (ID %d)"), name.c_str(), e.isActive ? OBF(" [đang bật]") : "", gpId);
        snap.guideCount++;
    }
    if (snap.guideCount > 1) sortGuides(snap.guides, snap.guideCount);
}

void buildFish(Snapshot &snap) {
    snap.fishCount = 0;
    Object *impl = getTableImpl(eTableType::Fishlist);
    int dictCount = readTableDictCount(impl);
    int visited = 0;
    int skippedType = 0;
    int skippedItem = 0;
    if (!impl) {
        logCatalogBuild(OBF("Fishlist"), impl, dictCount, visited, snap.fishCount);
        return;
    }
    forEachDictValues(impl, kMaxFish, [&](Object *row) -> bool {
        visited++;
        if (snap.fishCount >= kMaxFish) return false;
        Class *rowCls = row->get_class();
        if (!rowCls || !rowCls->find_method(OBF("get_ItemId"), 0)) return false;
        if (!rowCls->find_method(OBF("get_FishType"), 0)) return false;
        unsigned int itemId = row->invoke_method<unsigned int>(OBF("get_ItemId"));
        if (itemId == 0) return false;
        int fishType = (int) row->invoke_method<unsigned int>(OBF("get_FishType"));
        if (fishType <= (int) Fish_Type::None || fishType >= (int) Fish_Type::End) {
            skippedType++;
            return false;
        }
        if (!resolveItemIsFish(itemId)) {
            skippedItem++;
            return false;
        }
        int grade = resolveItemGrade(itemId);
        bool codex = resolveInCodex(itemId);
        std::string name = resolveItemName(itemId);
        FishEntry &e = snap.fish[snap.fishCount];
        e.itemId = itemId;
        e.grade = grade;
        e.fishType = fishType;
        e.inCodex = codex;
        copyLabelFmt(e.label, kLabelLen, OBF("%s (%u) %s %s%s"), name.c_str(), itemId, GradeTag(grade), FishTypeTag(fishType), codex ? OBF("") : OBF(" [thiếu codex]"));
        snap.fishCount++;
        return true;
    });
    static long long s_lastFishDetailLogMs = 0;
    long long now = Tools::getSystemMilliseconds();
    if (now - s_lastFishDetailLogMs >= 7000) {
        s_lastFishDetailLogMs = now;
        LOGI(OBF("FishingCatalog Fishlist: skippedType=%d skippedNotFish=%d"), skippedType, skippedItem);
    }
    logCatalogBuild(OBF("Fishlist"), impl, dictCount, visited, snap.fishCount);
}

void publishSnapshot(const Snapshot &snap) {
    int writeIdx = 1 - g_readIndex.load(std::memory_order_relaxed);
    g_buffers[writeIdx] = snap;
    g_buffers[writeIdx].ready = true;
    g_readIndex.store(writeIdx, std::memory_order_release);
    g_ready.store(true, std::memory_order_release);
}

bool shouldRebuildNow(int mapId, long long now) {
    if (!g_ready.load(std::memory_order_acquire)) return true;
    if (g_forceRebuild.exchange(false, std::memory_order_acq_rel)) return true;
    if (g_pickerOpen.load(std::memory_order_relaxed) && now - g_lastRebuildMs >= 2000) return true;
    if (mapId != g_lastMapId) return true;
    if (now - g_lastRebuildMs >= 7000) return true;
    return false;
}

}

void UpdateFromGameThread() {
    if (!il2cpp_loaded.load() || isGameLoading) return;
    long long now = Tools::getSystemMilliseconds();
    int mapId = CacheUser::myCurrentMapID();
    if (!shouldRebuildNow(mapId, now)) return;
    Snapshot snap{};
    snap.mapId = mapId;
    snap.ready = true;
    unsigned int castZone = AutoFishing::GetCastingZoneId();
    unsigned int catchZone = AutoFishing::GetCatchZoneId();
    long long baitUid = AutoFishing::GetFishingBaitUid();
    int activeGuide = readActiveGuidePoint();
    buildBaits(snap, baitUid);
    buildZones(snap, castZone, catchZone);
    buildGuides(snap, activeGuide);
    bool rebuildFish = !g_fishSessionBuilt || g_pickerOpen.load(std::memory_order_relaxed);
    if (!rebuildFish && g_ready.load(std::memory_order_acquire)) {
        int idx = g_readIndex.load(std::memory_order_acquire);
        if (g_buffers[idx].fishCount == 0) rebuildFish = true;
    }
    if (rebuildFish) {
        buildFish(snap);
        if (snap.fishCount > 0) g_fishSessionBuilt = true;
    } else {
        int idx = g_readIndex.load(std::memory_order_acquire);
        snap.fishCount = g_buffers[idx].fishCount;
        memcpy(snap.fish, g_buffers[idx].fish, sizeof(snap.fish));
    }
    g_lastRebuildMs = now;
    g_lastMapId = mapId;
    publishSnapshot(snap);
}

void RequestRebuild() {
    g_forceRebuild.store(true, std::memory_order_release);
}

void NotifyPickerOpen() {
    g_pickerOpen.store(true, std::memory_order_release);
    g_forceRebuild.store(true, std::memory_order_release);
}

void Read(Snapshot &out) {
    out.ready = false;
    if (!g_ready.load(std::memory_order_acquire)) return;
    int idx = g_readIndex.load(std::memory_order_acquire);
    out = g_buffers[idx];
}

const char *GradeTag(int grade) {
    switch (grade) {
        case 1: return OBF("C");
        case 2: return OBF("B");
        case 3: return OBF("A");
        case 4: return OBF("S");
        case 5: return OBF("SS");
        default: return OBF("-");
    }
}

const char *FishTypeTag(int fishType) {
    switch (fishType) {
        case (int) Fish_Type::NormalFish: return OBF("Thường");
        case (int) Fish_Type::KingFish: return OBF("Vua");
        case (int) Fish_Type::Junk: return OBF("Rác");
        case (int) Fish_Type::Costume: return OBF("Trang phục");
        case (int) Fish_Type::Vehicle: return OBF("Phương tiện");
        case (int) Fish_Type::MyHome: return OBF("Nhà");
        case (int) Fish_Type::Season: return OBF("Mùa");
        case (int) Fish_Type::Currency: return OBF("Tiền tệ");
        default: return OBF("?");
    }
}

}
