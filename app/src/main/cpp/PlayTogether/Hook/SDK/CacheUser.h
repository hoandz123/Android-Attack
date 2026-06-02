#ifndef SDK_CACHEUSER_H
#define SDK_CACHEUSER_H

#include <API/Il2CppApi.h>

namespace CacheUser {
    Class *get_class();
    Object *get_Instance();
    int myCurrentMapID();
    int GetItemCount(unsigned int itemId, bool ignoreEquip = true);
    long long GetItemUid(unsigned int itemId);
    int GetItemTypeCount(int itemType);
    int GetInventoryLimitFish();
    bool IsItemLocked(unsigned int itemId);
}

#endif // SDK_CACHEUSER_H
