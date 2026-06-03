#include "PickerSnapshot.h"
#include "AutoFishing.h"
#include <atomic>
#include "../../Config/Config.h"
#include "../SDK/CacheUser.h"
#include "../SDK/TableItemImpl.h"
#include "../SDK/TableMessagesImpl.h"
#include "../SDK/TableRecipeImpl.h"
#include "../SDK/TableSystem.h"
#include "../SDK/enum/eTableType.h"
#include "../SDK/enum/Item_Type.h"
#include <API/Il2CppApi.h>
#include <API/Il2cpp_Struct.h>
#include <Tools/Tools.h>
#include <FileManager.hpp>
#include <json/json.hpp>
#define LOGGER_TAG "ATTACK_PickerSnapshot"
#include <Includes/Logger.h>

extern bool isGameLoading;

namespace AutoFishing {

namespace {

// Double-buffer snapshot + cờ rebuild (UI thread đọc, game thread ghi).
std::atomic<bool> g_ready{false};
std::atomic<int> g_readIndex{0};
std::atomic<bool> g_forceRebuild{false};
std::atomic<bool> g_pickerOpen{false};
std::atomic<bool> g_craftPanelVisible{false};
PickerSnapshot g_buffers[2]{};
long long g_lastRebuildMs = 0;
long long g_lastBaitCountRefreshMs = 0;
int g_lastMapId = -1;

static bool g_baitCatalogReady = false;
static bool g_baitCatalogLiveBuilt = false;
static int g_baitCatalogCount = 0;
PickerBaitRecipeEntry g_baitCatalog[kPickerMaxBaitRecipes]{};

static bool g_baitTypeCacheReady = false;
static bool g_baitTypeCacheLiveBuilt = false;
static int g_baitTypeCacheCount = 0;
PickerBaitEntry g_baitTypeCache[kPickerMaxBaits]{};

static bool g_zoneCacheReady = false;
static bool g_zoneCacheLiveBuilt = false;
static int g_zoneCacheCount = 0;
PickerZoneEntry g_zoneCache[kPickerMaxZones]{};

static std::string pickerDataPath(const char *fileName) {
    static std::string dir;
    if (dir.empty()) {
        std::string pkg = Tools::GetPackageName();
        if (pkg.empty()) pkg = OBF("com.vng.playtogether");
        dir = OBF("/storage/emulated/0/Android/data/") + pkg + OBF("/");
    }
    return dir + fileName;
}

static bool readJsonFile(const char *fileName, nlohmann::json &out) {
    try {
        const std::string path = pickerDataPath(fileName);
        if (path.empty() || !fs::Exists(path)) return false;
        fs::Result rr;
        std::vector<uint8_t> bytes = fs::ReadBytes(path, &rr);
        if (!rr.ok() || bytes.empty()) return false;
        out = nlohmann::json::parse(std::string(bytes.begin(), bytes.end()));
        return true;
    } catch (...) {
        return false;
    }
}

static void writeJsonFile(const char *fileName, const nlohmann::json &j) {
    try {
        const std::string path = pickerDataPath(fileName);
        if (path.empty()) return;
        const std::string content = j.dump();
        fs::WriteBytes(path, content.data(), content.size());
    } catch (...) {
    }
}

static void setBaitLabel(PickerBaitRecipeEntry &e, const char *name, int owned) {
    e.ownedCount = owned;
    snprintf(e.label, kPickerLabelLen, OBF("%s · có %d"), name, owned);
}

static void formatBaitLabel(PickerBaitRecipeEntry &e) {
    setBaitLabel(e, TableItemImpl::GetDisplayName(e.itemId).c_str(), CacheUser::GetItemCount(e.itemId, true));
}

static bool loadBaitCatalogCache() {
    nlohmann::json j;
    if (!readJsonFile(OBF("bait_catalog.json"), j) || !j.contains("entries") || !j["entries"].is_array()) return false;
    int n = 0;
    for (const auto &row : j["entries"]) {
        if (n >= kPickerMaxBaitRecipes) break;
        if (!row.contains("itemId") || !row.contains("recipeId") || !row.contains("name")) continue;
        unsigned int itemId = row["itemId"].get<unsigned int>();
        unsigned int recipeId = row["recipeId"].get<unsigned int>();
        if (itemId == 0 || recipeId == 0) continue;
        PickerBaitRecipeEntry &e = g_baitCatalog[n++];
        e.itemId = itemId;
        e.recipeId = recipeId;
        setBaitLabel(e, row["name"].get<std::string>().c_str(), 0);
    }
    if (n <= 0) return false;
    g_baitCatalogCount = n;
    return true;
}

static void saveBaitCatalogCache() {
    if (g_baitCatalogCount <= 0) return;
    nlohmann::json entries = nlohmann::json::array();
    for (int i = 0; i < g_baitCatalogCount; i++) {
        const PickerBaitRecipeEntry &e = g_baitCatalog[i];
        entries.push_back({
            {"itemId", e.itemId},
            {"recipeId", e.recipeId},
            {"name", TableItemImpl::GetDisplayName(e.itemId)},
        });
    }
    writeJsonFile(OBF("bait_catalog.json"), nlohmann::json{{"entries", std::move(entries)}});
}

static bool loadBaitTypeCache() {
    nlohmann::json j;
    if (!readJsonFile(OBF("bait_types.json"), j) || !j.contains("entries") || !j["entries"].is_array()) return false;
    int n = 0;
    for (const auto &row : j["entries"]) {
        if (n >= kPickerMaxBaits) break;
        if (!row.contains("itemId") || !row.contains("name")) continue;
        unsigned int itemId = row["itemId"].get<unsigned int>();
        if (itemId == 0) continue;
        PickerBaitEntry &e = g_baitTypeCache[n++];
        e.itemId = itemId;
        snprintf(e.label, kPickerLabelLen, "%s", row["name"].get<std::string>().c_str());
    }
    if (n <= 0) return false;
    g_baitTypeCacheCount = n;
    return true;
}

static void saveBaitTypeCache() {
    if (g_baitTypeCacheCount <= 0) return;
    nlohmann::json entries = nlohmann::json::array();
    for (int i = 0; i < g_baitTypeCacheCount; i++) {
        const PickerBaitEntry &e = g_baitTypeCache[i];
        entries.push_back({{"itemId", e.itemId}, {"name", TableItemImpl::GetDisplayName(e.itemId)}});
    }
    writeJsonFile(OBF("bait_types.json"), nlohmann::json{{"entries", std::move(entries)}});
}

static bool loadZoneCache() {
    nlohmann::json j;
    if (!readJsonFile(OBF("zone_catalog.json"), j) || !j.contains("entries") || !j["entries"].is_array()) return false;
    int n = 0;
    for (const auto &row : j["entries"]) {
        if (n >= kPickerMaxZones) break;
        if (!row.contains("zoneId") || !row.contains("name")) continue;
        unsigned int zoneId = row["zoneId"].get<unsigned int>();
        if (zoneId == 0) continue;
        PickerZoneEntry &e = g_zoneCache[n++];
        e.zoneId = zoneId;
        snprintf(e.label, kPickerLabelLen, "%s", row["name"].get<std::string>().c_str());
    }
    if (n <= 0) return false;
    g_zoneCacheCount = n;
    return true;
}

static void saveZoneCache() {
    if (g_zoneCacheCount <= 0) return;
    nlohmann::json entries = nlohmann::json::array();
    for (int i = 0; i < g_zoneCacheCount; i++) {
        const PickerZoneEntry &e = g_zoneCache[i];
        entries.push_back({{"zoneId", e.zoneId}, {"name", e.label}});
    }
    writeJsonFile(OBF("zone_catalog.json"), nlohmann::json{{"entries", std::move(entries)}});
}

static unsigned int resolveBaitRecipe(unsigned int itemId, int /*staticProductId*/) {
    return TableRecipeImpl::ResolveRecipeId(itemId);
}

static bool appendBaitRecipe(PickerSnapshot &snap, unsigned int itemId, unsigned int recipeId) {
    if (itemId == 0 || recipeId == 0) return false;
    for (int i = 0; i < snap.baitRecipeCount; i++) {
        if (snap.baitRecipes[i].itemId == itemId) return false;
    }
    if (snap.baitRecipeCount >= kPickerMaxBaitRecipes) return false;
    PickerBaitRecipeEntry &e = snap.baitRecipes[snap.baitRecipeCount++];
    e.itemId = itemId;
    e.recipeId = recipeId;
    formatBaitLabel(e);
    return true;
}

static void ensureTableLoaded(eTableType type, Object *impl) {
    if (!impl) return;
    if (TableSystem::GetEntryCount(impl) > 0) return;
    TableSystem::TryForceLoad(type, impl);
    if (TableSystem::GetEntryCount(impl) <= 0) {
        TableSystem::LoadBuildToTable(impl);
        if (TableSystem::GetEntryCount(impl) <= 0) {
            TableSystem::LoadTable(type, impl);
        }
    }
}

static void collectBaitTypeEntries(PickerSnapshot &snap) {
    Object *baitImpl = TableSystem::GetTableUnit(eTableType::FishingBait);
    ensureTableLoaded(eTableType::FishingBait, baitImpl);
    if (!baitImpl) return;
    TableSystem::ForEachListRow(baitImpl, kPickerMaxBaits * 2, [&](Object *row) -> bool {
        if (snap.baitCount >= kPickerMaxBaits) return false;
        Class *rowCls = row->get_class();
        if (!rowCls || !rowCls->find_method(OBF("get_BaitItemId"), 0)) return true;
        unsigned int itemId = row->invoke_method<unsigned int>(OBF("get_BaitItemId"));
        if (itemId == 0) return true;
        for (int i = 0; i < snap.baitCount; i++) {
            if (snap.baits[i].itemId == itemId) return true;
        }
        PickerBaitEntry &e = snap.baits[snap.baitCount++];
        e.itemId = itemId;
        std::string name = TableItemImpl::GetDisplayName(itemId);
        snprintf(e.label, kPickerLabelLen, "%s", name.c_str());
        return true;
    });
}

static void collectZoneEntries(PickerSnapshot &snap, unsigned int castingZone, unsigned int catchZone) {
    Object *areaImpl = TableSystem::GetTableUnit(eTableType::FishingArea);
    ensureTableLoaded(eTableType::FishingArea, areaImpl);
    if (!areaImpl) return;
    int cap = TableSystem::GetDictCount(areaImpl);
    if (cap <= 0) cap = kPickerMaxZones;
    TableSystem::ForEachDictValue(areaImpl, cap, [&](Object *row) -> bool {
        if (snap.zoneCount >= kPickerMaxZones) return false;
        Class *rowCls = row->get_class();
        if (!rowCls || !rowCls->find_method(OBF("get_FishingZoneId"), 0)) return false;
        unsigned int zoneId = row->invoke_method<unsigned int>(OBF("get_FishingZoneId"));
        if (zoneId == 0) return false;
        unsigned int zoneTextId = 0;
        if (rowCls->find_method(OBF("get_FishingZoneText"), 0)) {
            zoneTextId = row->invoke_method<unsigned int>(OBF("get_FishingZoneText"));
        }
        std::string name = TableMessagesImpl::GetMessageDx(zoneTextId);
        if (name.empty()) return true;
        for (int i = 0; i < snap.zoneCount; i++) {
            if (snap.zones[i].zoneId == zoneId) return true;
        }
        PickerZoneEntry &e = snap.zones[snap.zoneCount++];
        e.zoneId = zoneId;
        snprintf(e.label, kPickerLabelLen, OBF("%s%s"), name.c_str(),
                 (zoneId == castingZone || zoneId == catchZone) ? OBF(" · đang ở") : "");
        return true;
    });
}

static void collectBaitCatalogEntries(PickerSnapshot &scratch) {
    Object *baitImpl = TableSystem::GetTableUnit(eTableType::FishingBait);
    ensureTableLoaded(eTableType::FishingBait, baitImpl);
    if (!baitImpl) return;
    TableSystem::ForEachListRow(baitImpl, kPickerMaxBaitRecipes * 2, [&](Object *row) -> bool {
        if (scratch.baitRecipeCount >= kPickerMaxBaitRecipes) return false;
        Class *rowCls = row->get_class();
        if (!rowCls || !rowCls->find_method(OBF("get_BaitItemId"), 0)) return true;
        unsigned int itemId = row->invoke_method<unsigned int>(OBF("get_BaitItemId"));
        if (itemId == 0) return true;
        int staticProductId = rowCls->find_method(OBF("get_StaticProductId"), 0)
                              ? row->invoke_method<int>(OBF("get_StaticProductId")) : 0;
        appendBaitRecipe(scratch, itemId, resolveBaitRecipe(itemId, staticProductId));
        return true;
    });
}

static void refreshBaitCatalogCounts() {
    for (int i = 0; i < g_baitCatalogCount; i++) formatBaitLabel(g_baitCatalog[i]);
}

static void ensureBaitTypeCache() {
    if (g_baitTypeCacheLiveBuilt) return;
    PickerSnapshot scratch{};
    collectBaitTypeEntries(scratch);
    if (scratch.baitCount <= 0) return;
    g_baitTypeCacheCount = scratch.baitCount;
    for (int i = 0; i < g_baitTypeCacheCount; i++) g_baitTypeCache[i] = scratch.baits[i];
    g_baitTypeCacheReady = true;
    g_baitTypeCacheLiveBuilt = true;
    saveBaitTypeCache();
    LOGI(OBF("PickerSnapshot bait types: count=%d"), g_baitTypeCacheCount);
}

static void ensureZoneCache() {
    if (g_zoneCacheLiveBuilt) return;
    PickerSnapshot scratch{};
    collectZoneEntries(scratch, 0, 0);
    if (scratch.zoneCount <= 0) return;
    g_zoneCacheCount = scratch.zoneCount;
    for (int i = 0; i < g_zoneCacheCount; i++) g_zoneCache[i] = scratch.zones[i];
    g_zoneCacheReady = true;
    g_zoneCacheLiveBuilt = true;
    saveZoneCache();
    LOGI(OBF("PickerSnapshot zones: count=%d"), g_zoneCacheCount);
}

static void ensureBaitCatalog() {
    if (g_baitCatalogLiveBuilt) return;
    const long long t0 = Tools::getSystemMilliseconds();
    PickerSnapshot scratch{};
    collectBaitCatalogEntries(scratch);
    if (scratch.baitRecipeCount <= 0) return;
    g_baitCatalogCount = scratch.baitRecipeCount;
    for (int i = 0; i < g_baitCatalogCount; i++) g_baitCatalog[i] = scratch.baitRecipes[i];
    g_baitCatalogReady = true;
    g_baitCatalogLiveBuilt = true;
    saveBaitCatalogCache();
    LOGI(OBF("PickerSnapshot bait catalog: craftable=%d ms=%lld"),
         g_baitCatalogCount, Tools::getSystemMilliseconds() - t0);
}

static void copyBaitRecipes(PickerSnapshot &snap) {
    ensureBaitCatalog();
    snap.baitRecipeCount = g_baitCatalogCount;
    for (int i = 0; i < g_baitCatalogCount; i++) snap.baitRecipes[i] = g_baitCatalog[i];
}

static void applyCachedSnapshot(PickerSnapshot &snap) {
    if (g_baitTypeCacheReady && g_baitTypeCacheCount > 0) {
        snap.baitCount = g_baitTypeCacheCount;
        for (int i = 0; i < g_baitTypeCacheCount; i++) snap.baits[i] = g_baitTypeCache[i];
    }
    if (g_zoneCacheReady && g_zoneCacheCount > 0) {
        snap.zoneCount = g_zoneCacheCount;
        for (int i = 0; i < g_zoneCacheCount; i++) snap.zones[i] = g_zoneCache[i];
    }
    if (g_baitCatalogReady && g_baitCatalogCount > 0) {
        snap.baitRecipeCount = g_baitCatalogCount;
        for (int i = 0; i < g_baitCatalogCount; i++) snap.baitRecipes[i] = g_baitCatalog[i];
    }
    snap.ready = snap.baitCount > 0 || snap.zoneCount > 0 || snap.baitRecipeCount > 0;
}

} // namespace

void InitPickerCache() {
    if (!g_baitCatalogReady && loadBaitCatalogCache()) g_baitCatalogReady = true;
    if (!g_baitTypeCacheReady && loadBaitTypeCache()) g_baitTypeCacheReady = true;
    if (!g_zoneCacheReady && loadZoneCache()) g_zoneCacheReady = true;
    LOGI(OBF("PickerSnapshot cache: baits=%d zones=%d craft=%d"),
         g_baitTypeCacheCount, g_zoneCacheCount, g_baitCatalogCount);
}

// Game thread: build snapshot mồi/vùng rồi publish cho UI.
void UpdatePickerFromGameThread() {
    if (isGameLoading) return;
    long long now = Tools::getSystemMilliseconds();
    int mapId = CacheUser::myCurrentMapID();

    // Bỏ qua rebuild nếu snapshot còn mới (trừ khi mở picker / đổi map / force).
    const bool livePanel = g_pickerOpen.load(std::memory_order_relaxed) ||
                           g_craftPanelVisible.load(std::memory_order_relaxed);
    if (g_ready.load(std::memory_order_acquire)) {
        if (!g_forceRebuild.exchange(false, std::memory_order_acq_rel)) {
            if (livePanel && now - g_lastBaitCountRefreshMs >= 5000) {
                refreshBaitCatalogCounts();
                g_lastBaitCountRefreshMs = now;
                int readIdx = g_readIndex.load(std::memory_order_relaxed);
                int writeIdx = 1 - readIdx;
                g_buffers[writeIdx] = g_buffers[readIdx];
                copyBaitRecipes(g_buffers[writeIdx]);
                g_buffers[writeIdx].ready = true;
                g_readIndex.store(writeIdx, std::memory_order_release);
                g_lastRebuildMs = now;
                return;
            }
            if (!(livePanel && now - g_lastRebuildMs >= 5000) &&
                mapId == g_lastMapId && now - g_lastRebuildMs < 15000) {
                return;
            }
        }
    }

    PickerSnapshot snap{};
    snap.ready = true;
    ensureBaitTypeCache();
    ensureZoneCache();
    const long long equippedUid = GetFishingBaitUid();
    const unsigned int castingZone = GetCastingZoneId();
    const unsigned int catchZone = GetCatchZoneId();

    // Danh sách mồi trong túi (CacheUser.GetItemList) cho combo UI.
    Object *cacheUser = CacheUser::get_Instance();
    if (cacheUser) {
        Class *cls = cacheUser->get_class();
        if (cls && cls->find_method(OBF("GetItemList"), 1)) {
            int baitType = (int) Item_Type::BaitItem;
            Object *rawList = cacheUser->invoke_method<Object *>(OBF("GetItemList"), baitType);
            if (rawList) {
                auto *list = (List<Object *> *) rawList;
                int count = list->get_Count();
                if (count > kPickerMaxBaits) count = kPickerMaxBaits;
                for (int i = 0; i < count && snap.baitCount < kPickerMaxBaits; i++) {
                    Object *item = list->get_item(i);
                    if (!item) continue;
                    Class *itemCls = item->get_class();
                    if (!itemCls || !itemCls->find_method(OBF("get_ItemID"), 0) ||
                        !itemCls->find_method(OBF("get_ItemUID"), 0) || !itemCls->find_method(OBF("get_ItemCount"), 0)) {
                        continue;
                    }
                    unsigned int itemId = item->invoke_method<unsigned int>(OBF("get_ItemID"));
                    int itemCount = item->invoke_method<int>(OBF("get_ItemCount"));
                    if (itemId == 0 || itemCount <= 0) continue;
                    long long uid = item->invoke_method<long long>(OBF("get_ItemUID"));
                    if (itemCls->find_method(OBF("get_IsLock"), 0) && item->invoke_method<bool>(OBF("get_IsLock"))) {
                        continue;
                    }
                    PickerBaitEntry &e = snap.baits[snap.baitCount++];
                    e.itemId = itemId;
                    std::string name = TableItemImpl::GetDisplayName(itemId);
                    snprintf(e.label, kPickerLabelLen, OBF("%s x%d%s"), name.c_str(), itemCount,
                             equippedUid != 0 && uid == equippedUid ? OBF(" · đang gắn") : "");
                }
            }
        }
    }
    if (snap.baitCount == 0 && g_baitTypeCacheReady && g_baitTypeCacheCount > 0) {
        snap.baitCount = g_baitTypeCacheCount;
        for (int i = 0; i < g_baitTypeCacheCount; i++) snap.baits[i] = g_baitTypeCache[i];
    }

    copyBaitRecipes(snap);
    {
        static long long s_lastRecipeLogMs = 0;
        if (now - s_lastRecipeLogMs >= 7000) {
            s_lastRecipeLogMs = now;
            Object *baitImpl = TableSystem::GetTableUnit(eTableType::FishingBait);
            Object *recipeImpl = TableSystem::GetTableUnit(eTableType::Recipe);
            LOGI(OBF("PickerSnapshot bait recipes: snap=%d baitTbl=%d recipeTbl=%d"),
                 snap.baitRecipeCount, TableSystem::GetEntryCount(baitImpl),
                 TableSystem::GetEntryCount(recipeImpl));
        }
    }

    // Danh sách vùng câu từ bảng FishingArea (đánh dấu vùng cast/catch hiện tại).
    int zoneVisited = 0;
    {
        Object *areaImpl = TableSystem::GetTableUnit(eTableType::FishingArea);
        zoneVisited = TableSystem::GetDictCount(areaImpl);
        collectZoneEntries(snap, castingZone, catchZone);
        if (snap.zoneCount == 0 && g_zoneCacheReady && g_zoneCacheCount > 0) {
            snap.zoneCount = g_zoneCacheCount;
            for (int i = 0; i < g_zoneCacheCount; i++) snap.zones[i] = g_zoneCache[i];
        }
    }
    // Log debug zone (tối đa 1 lần / 7s).
    {
        static long long s_lastZoneLogMs = 0;
        if (now - s_lastZoneLogMs >= 7000) {
            s_lastZoneLogMs = now;
            LOGI(OBF("PickerSnapshot FishingArea: dictCount=%d snap=%d"), zoneVisited, snap.zoneCount);
        }
    }

    // Xuất bản snapshot sang buffer phụ (UI đọc buffer đang active).
    int writeIdx = 1 - g_readIndex.load(std::memory_order_relaxed);
    g_buffers[writeIdx] = snap;
    g_buffers[writeIdx].ready = true;
    g_readIndex.store(writeIdx, std::memory_order_release);
    g_ready.store(true, std::memory_order_release);
    g_lastRebuildMs = now;
    g_lastMapId = mapId;
}

// Đánh dấu rebuild ở tick kế (sau khi config/thay đổi khác).
void RequestPickerRebuild() {
    g_forceRebuild.store(true, std::memory_order_release);
}

// UI mở combo picker — rebuild ngay lần đầu.
void NotifyPickerOpen() {
    g_pickerOpen.store(true, std::memory_order_release);
    g_forceRebuild.store(true, std::memory_order_release);
}

// Tab chế mồi đang hiển thị — chỉ bật cờ, không ép rebuild.
void NotifyCraftPanelVisible() {
    g_craftPanelVisible.store(true, std::memory_order_release);
}

void NotifyPickerClosed() {
    g_pickerOpen.store(false, std::memory_order_release);
    g_craftPanelVisible.store(false, std::memory_order_release);
}

// UI thread: copy snapshot đã publish (không gọi IL2CPP).
void ReadPicker(PickerSnapshot &out) {
    out.ready = false;
    if (g_ready.load(std::memory_order_acquire)) {
        out = g_buffers[g_readIndex.load(std::memory_order_acquire)];
        return;
    }
    applyCachedSnapshot(out);
}

unsigned int LookupBaitCraftRecipeId(unsigned int itemId) {
    if (itemId == 0) return 0;
    for (int i = 0; i < g_baitCatalogCount; i++)
        if (g_baitCatalog[i].itemId == itemId) return g_baitCatalog[i].recipeId;
    if (g_baitCatalogLiveBuilt) return TableRecipeImpl::ResolveRecipeId(itemId);
    return 0;
}

}
