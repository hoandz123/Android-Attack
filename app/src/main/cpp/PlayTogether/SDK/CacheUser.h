#ifndef SDK_CACHEUSER_H
#define SDK_CACHEUSER_H

#include <API/Il2CppApi.h>
#include "enum/Item_Type.h"

namespace CacheUser {
    Class *get_class();
    Object *get_Instance();
    int myCurrentMapID();
    Object* GetItem(int itemID);
    List<Object *> *GetItemList(Item_Type type);
    int GetCount(int itmID);
    int GetItemTypeCount(Item_Type type);
    int GetCount(Item_Type type, int minGrade);
    void ItemSell(Item_Type type, int minGrade);
}

#endif // SDK_CACHEUSER_H
