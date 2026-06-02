#include "CacheUser.h"
#include "CacheSystem.h"
#include "enum/Item_Type.h"

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
    Object *GetItem(int itemID) {
        Object *instance = get_Instance();
        if (!instance) return nullptr;
        static auto _ = (Object* (*)(void*, int)) IL2Cpp::Il2CppGetMethodOffset("Assembly-CSharp.dll", "", "CacheUser", "GetItem", 1, 2);
        return _(instance, itemID);
    }

    List<Object *> *GetItemList(Item_Type type) {
        Object *instance = get_Instance();
        if (!instance) return nullptr;
        return instance->invoke_method<List<Object *> *>("GetItemList", type);
    }
    int GetItemTypeCount(Item_Type type) {
        List<Object *> *itemList = GetItemList(type);
        if (itemList) {
            return itemList->get_Count();
        }
        return 0;
    }

    int GetCount(int itmID) {
        int count = 0;
        for (int j = 0; j < 100; ++j) {
            List<Object*>* _sellItemList = GetItemList((Item_Type)j);
            if (_sellItemList != NULL) {
                int size = _sellItemList->get_Count();
                static uintptr_t offsetID = IL2Cpp::Il2CppGetFieldOffset("PlayTogether", "UserItem", "<ItemID>k__BackingField");
                for (int i = 0; i < size; ++i) {
                    void* item = _sellItemList->get_item(i);
                    if (!item) {
                        continue;
                    }
                    int ItemID = *(int*)((uintptr_t)item + offsetID);
                    if (itmID == ItemID) {
                        int ItemCount = *(int*)((uintptr_t)item + IL2Cpp::Il2CppGetFieldOffset("PlayTogether", "UserItem", "<ItemCount>k__BackingField"));
                        count += ItemCount;
                    }
                }
            }
        }
        return count;
    }

    // TODO G3: port GetCount(type, minGrade) — cần TableSystem::GetTableUnit + eTableType
    int GetCount(Item_Type type, int minGrade) {
        return 0;
    }
    // TODO G3: port ItemSell — cần TableSystem + NetNativeProtocol::SendToItemSell + RemoveItemInfo
    void ItemSell(Item_Type type, int minGrade) {
    }
}
