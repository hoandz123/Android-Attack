#include "CacheUser.h"
#include "CacheSystem.h"
#define LOGGER_TAG "ATTACK_PlayTogether"
#include <Includes/Logger.h>

namespace CacheUser {
    Class *get_class() {
        return FindClass("CacheUser");
    }
    Object *get_Instance() {
        return CacheSystem::get_CacheUser();
    }
    int myCurrentMapID() {
        Object *instance = get_Instance();
        if (!instance) return 0;
        return instance->get_field_value<int>("myCurrentMapID");
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
}
