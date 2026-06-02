#include "PickerSnapshot.h"
#include "AutoFishing.h"
#include <atomic>
#include "../../Config/Config.h"
#include "../SDK/CacheUser.h"
#include "../SDK/TableItemImpl.h"
#include "../SDK/TableMessagesImpl.h"
#include "../SDK/TableSystem.h"
#include "../SDK/enum/eTableType.h"
#include "../SDK/enum/Item_Type.h"
#include <API/Il2CppApi.h>
#include <API/Il2cpp_Struct.h>
#include <Tools/Tools.h>
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
PickerSnapshot g_buffers[2]{};
long long g_lastRebuildMs = 0;
int g_lastMapId = -1;

} // namespace

// Game thread: build snapshot mồi/vùng rồi publish cho UI.
void UpdatePickerFromGameThread() {
    if (isGameLoading) return;
    long long now = Tools::getSystemMilliseconds();
    int mapId = CacheUser::myCurrentMapID();

    // Bỏ qua rebuild nếu snapshot còn mới (trừ khi mở picker / đổi map / force).
    if (g_ready.load(std::memory_order_acquire)) {
        if (!g_forceRebuild.exchange(false, std::memory_order_acq_rel)) {
            if (!(g_pickerOpen.load(std::memory_order_relaxed) && now - g_lastRebuildMs >= 2000) &&
                mapId == g_lastMapId && now - g_lastRebuildMs < 7000) {
                return;
            }
        }
    }

    PickerSnapshot snap{};
    snap.ready = true;
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
                    bool locked = itemCls->find_method(OBF("get_IsLock"), 0) && item->invoke_method<bool>(OBF("get_IsLock"));
                    PickerBaitEntry &e = snap.baits[snap.baitCount++];
                    e.itemId = itemId;
                    std::string name = TableItemImpl::GetDisplayName(itemId);
                    snprintf(e.label, kPickerLabelLen, OBF("%s (%u) x%d%s%s"), name.c_str(), itemId, itemCount,
                             equippedUid != 0 && uid == equippedUid ? OBF(" [gắn]") : "",
                             locked ? OBF(" [khóa]") : "");
                }
            }
        }
    }

    // Danh sách vùng câu từ bảng FishingArea (đánh dấu vùng cast/catch hiện tại).
    Object *areaImpl = TableSystem::GetTableUnit(eTableType::FishingArea);
    int zoneVisited = 0;
    if (areaImpl) {
        TableSystem::ForEachDictValue(areaImpl, kPickerMaxZones, [&](Object *row) -> bool {
            zoneVisited++;
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
            snprintf(e.label, kPickerLabelLen, OBF("%s%s (%u)"), name.c_str(),
                     (zoneId == castingZone || zoneId == catchZone) ? OBF(" [hiện tại]") : "", zoneId);
            return true;
        });
    }
    // Log debug zone (tối đa 1 lần / 7s).
    {
        static long long s_lastZoneLogMs = 0;
        if (now - s_lastZoneLogMs >= 7000) {
            s_lastZoneLogMs = now;
            LOGI(OBF("PickerSnapshot FishingArea: impl=%p dictCount=%d visited=%d snap=%d"), areaImpl,
                 TableSystem::GetDictCount(areaImpl), zoneVisited, snap.zoneCount);
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

// UI mở combo picker — rebuild nhanh hơn (2s) trong khi picker mở.
void NotifyPickerOpen() {
    g_pickerOpen.store(true, std::memory_order_release);
    g_forceRebuild.store(true, std::memory_order_release);
}

// UI thread: copy snapshot đã publish (không gọi IL2CPP).
void ReadPicker(PickerSnapshot &out) {
    out.ready = false;
    if (!g_ready.load(std::memory_order_acquire)) return;
    out = g_buffers[g_readIndex.load(std::memory_order_acquire)];
}

}
