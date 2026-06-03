#include "CacheUser.h"
#include "CacheSystem.h"
#include "enum/Item_Type.h"
#include <API/Il2cpp_Struct.h>
#define LOGGER_TAG "ATTACK_PlayTogether"
#include <Includes/Logger.h>

namespace CacheUser {
    Class *get_class() {
        return FindClass(OBF("CacheUser"));
    }
    Object *get_Instance() {
        return CacheSystem::get_CacheUser();
    }
    int myCurrentMapID() {
        Object *instance = get_Instance();
        if (!instance) return 0;
        return instance->get_field_value<int>(OBF("myCurrentMapID"));
    }

    int GetItemCount(unsigned int itemId, bool ignoreEquip) {
        Object *instance = get_Instance();
        if (!instance || itemId == 0) return 0;
        Class *cls = get_class();
        if (!cls || !cls->find_method(OBF("GetItemCount"), 2)) return 0;
        return instance->invoke_method<int>(OBF("GetItemCount"), itemId, ignoreEquip);
    }

    long long GetItemUid(unsigned int itemId) {
        Object *instance = get_Instance();
        if (!instance || itemId == 0) return 0;
        Class *cls = get_class();
        if (!cls || !cls->find_method(OBF("GetItem"), 1)) return 0;
        Object *item = instance->invoke_method<Object *>(OBF("GetItem"), itemId);
        if (!item) return 0;
        return item->invoke_method<long long>(OBF("get_ItemUID"));
    }

    static long long uidFromUserItem(Object *item) {
        if (!item) return 0;
        Class *itemCls = item->get_class();
        if (!itemCls || !itemCls->find_method(OBF("get_ItemUID"), 0) ||
            !itemCls->find_method(OBF("get_ItemCount"), 0)) {
            return 0;
        }
        if (itemCls->find_method(OBF("get_IsLock"), 0) && item->invoke_method<bool>(OBF("get_IsLock"))) {
            return 0;
        }
        if (item->invoke_method<int>(OBF("get_ItemCount")) <= 0) return 0;
        return item->invoke_method<long long>(OBF("get_ItemUID"));
    }

    long long GetBaitItemUid(unsigned int itemId) {
        Object *instance = get_Instance();
        if (!instance || itemId == 0) return 0;
        Class *cls = get_class();
        if (!cls) return 0;

        if (cls->find_method(OBF("GetItem"), 1)) {
            Object *item = instance->invoke_method<Object *>(OBF("GetItem"), itemId);
            long long uid = uidFromUserItem(item);
            if (uid != 0) return uid;
        }

        if (!cls->find_method(OBF("GetItemList"), 1)) return 0;
        int baitType = (int) Item_Type::BaitItem;
        Object *rawList = instance->invoke_method<Object *>(OBF("GetItemList"), baitType);
        if (!rawList) return 0;
        auto *list = (List<Object *> *) rawList;
        long long bestUid = 0;
        int bestCount = 0;
        for (int i = 0, count = list->get_Count(); i < count; i++) {
            Object *item = list->get_item(i);
            if (!item) continue;
            Class *itemCls = item->get_class();
            if (!itemCls || !itemCls->find_method(OBF("get_ItemID"), 0)) continue;
            if (item->invoke_method<unsigned int>(OBF("get_ItemID")) != itemId) continue;
            long long uid = uidFromUserItem(item);
            if (uid == 0) continue;
            int itemCount = item->invoke_method<int>(OBF("get_ItemCount"));
            if (itemCount > bestCount) {
                bestCount = itemCount;
                bestUid = uid;
            }
        }
        return bestUid;
    }

    int GetItemTypeCount(int itemType) {
        Object *instance = get_Instance();
        if (!instance) return 0;
        Class *cls = get_class();
        if (!cls || !cls->find_method(OBF("GetItemTypeCount"), 1)) return 0;
        return instance->invoke_method<int>(OBF("GetItemTypeCount"), itemType);
    }

    int GetInventoryLimitFish() {
        Object *instance = get_Instance();
        if (!instance) return 0;
        Object *cfg = instance->get_field_object<Object *>(OBF("configData"));
        if (!cfg) return 0;
        Class *cfgCls = cfg->get_class();
        if (!cfgCls || !cfgCls->find_method(OBF("get_InventoryLimitFish"), 0)) return 0;
        return cfg->invoke_method<int>(OBF("get_InventoryLimitFish"));
    }

    bool IsItemLocked(unsigned int itemId) {
        Object *instance = get_Instance();
        if (!instance || itemId == 0) return false;
        Class *cls = get_class();
        if (!cls || !cls->find_method(OBF("GetItem"), 1)) return false;
        Object *item = instance->invoke_method<Object *>(OBF("GetItem"), itemId);
        if (!item) return false;
        Class *itemCls = item->get_class();
        if (!itemCls || !itemCls->find_method(OBF("get_IsLock"), 0)) return false;
        return item->invoke_method<bool>(OBF("get_IsLock"));
    }

    bool SetRewardPack(Object *rewardPack, bool coinLateUpdate) {
        Object *instance = get_Instance();
        if (!instance || !rewardPack) return false;
        Class *cls = get_class();
        if (!cls || !cls->find_method(OBF("SetRewardPack"), 2)) return false;
        instance->invoke_method<void>(OBF("SetRewardPack"), rewardPack, coinLateUpdate);
        return true;
    }
}
