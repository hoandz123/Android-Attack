#include "CacheUser.h"
#include "TableSystem.h"
#include "CacheSystem.h"
#include "NetNativeProtocol.h"
#include "enum/Item_Type.h"
#include "enum/eTableType.h"
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
    Object *GetItem(int itemID) {
        Object *instance = get_Instance();
        if (!instance) return nullptr;
        static auto _ = (Object *(*)(void *, int)) IL2Cpp::Il2CppGetMethodOffset("Assembly-CSharp.dll", "", "CacheUser", "GetItem", 1, 2);
        return _(instance, itemID);
    }
    List<Object *> *GetItemList(Item_Type type) {
        Object *instance = get_Instance();
        if (!instance) return nullptr;
        return instance->invoke_method<List<Object *> *>("GetItemList", type);
    }
    int GetItemTypeCount(Item_Type type) {
        List<Object *> *itemList = GetItemList(type);
        if (itemList) return itemList->get_Count();
        return 0;
    }
    int GetCount(int itmID) {
        int count = 0;
        for (int j = 0; j < 100; ++j) {
            List<Object *> *_sellItemList = GetItemList((Item_Type) j);
            if (_sellItemList != NULL) {
                int size = _sellItemList->get_Count();
                static uintptr_t offsetID = IL2Cpp::Il2CppGetFieldOffset("PlayTogether", "UserItem", "<ItemID>k__BackingField");
                for (int i = 0; i < size; ++i) {
                    void *item = _sellItemList->get_item(i);
                    if (!item) continue;
                    int ItemID = *(int *) ((uintptr_t) item + offsetID);
                    if (itmID == ItemID) {
                        int ItemCount = *(int *) ((uintptr_t) item + IL2Cpp::Il2CppGetFieldOffset("PlayTogether", "UserItem", "<ItemCount>k__BackingField"));
                        count += ItemCount;
                    }
                }
            }
        }
        return count;
    }
    int GetCount(Item_Type type, int minGrade) {
        int count = 0;
        List<Object *> *_sellItemList = GetItemList(type);
        if (_sellItemList != NULL) {
            for (int i = 0; i < _sellItemList->get_Count(); ++i) {
                Object *item = _sellItemList->get_item(i);
                if (!item) continue;
                int ItemID = item->get_field_value<int>("<ItemID>k__BackingField");
                Object *TItem = TableSystem::GetTableUnit<Object *>(FindClass("PlayTogether.Table.eTableType")->get_enum_value<eTableType>("Item"));
                if (!TItem) {
                    LOGE(OBF("TableItemImpl is NULL"));
                    continue;
                }
                int grade = TItem->invoke_method<int>("GetGrade", ItemID);
                if (grade < minGrade) count++;
            }
        }
        return count;
    }
    void ItemSell(Item_Type type, int minGrade) {
        List<Object *> *_sellItemList = GetItemList(type);
        if (_sellItemList != NULL) {
            List<Object *> *_selectedItemInfoList = (List<Object *> *) FindClass("System.Collections.Generic.List<PlayTogether.RemoveItemInfo>")->new_object();
            if (_selectedItemInfoList) {
                int size = _sellItemList->get_Count();
                for (int i = 0; i < size; ++i) {
                    Object *item = _sellItemList->get_item(i);
                    if (!item) continue;
                    long ItemUID = item->get_field_value<long>("<ItemUID>k__BackingField");
                    int ItemID = item->get_field_value<int>("<ItemID>k__BackingField");
                    int ItemCount = item->get_field_value<int>("<ItemCount>k__BackingField");
                    Object *TItem = TableSystem::GetTableUnit<Object *>(eTableType::Item);
                    if (!TItem) continue;
                    int grade = TItem->invoke_method<int>("GetGrade", ItemID);
                    if (grade < minGrade) {
                        Object *newRemoveItemInfo = FindClass("PlayTogether.RemoveItemInfo")->new_object();
                        if (newRemoveItemInfo) {
                            newRemoveItemInfo->set_field_value("<ItemUID>k__BackingField", ItemUID);
                            newRemoveItemInfo->set_field_value("<ItemCount>k__BackingField", ItemCount);
                            _selectedItemInfoList->Add(newRemoveItemInfo);
                        }
                        if (_selectedItemInfoList->get_Count() >= 30) {
                            NetNativeProtocol::SendToItemSell(_selectedItemInfoList, Item_Type::Insect, 13020069);
                            _selectedItemInfoList->Clear();
                        }
                    }
                }
                if (_selectedItemInfoList->get_Count() > 0) {
                    NetNativeProtocol::SendToItemSell(_selectedItemInfoList, Item_Type::Insect, 13020069);
                }
            }
        }
    }
}
