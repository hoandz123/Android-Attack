#include "CacheUser.h"
#include "CacheSystem.h"
#include <Includes/obfuscate.h>
#define LOG_TAG OBF("ATTACK_PlayTogether")
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
}
