#include "TableItemImpl.h"
#include <API/Il2CppApi.h>
#include "enum/Item_Type.h"
#include "TableSystem.h"
#include <string>

namespace TableItemImpl {
    Class *get_class() {
        return FindClass("TableItemImpl");
    }
    Object *get_Instance() {
        return TableSystem::GetTableUnit<Object *>(FindClass("PlayTogether.Table.eTableType")->get_enum_value<eTableType>("Item"));
    }
    Object *GetTableData(int sid) {
        Object *instance = get_Instance();
        if (!instance) return nullptr;
        return instance->invoke_method<Object *>("GetTableData", sid);
    }
    std::string GetAssetName(int sid) {
        Object *instance = get_Instance();
        if (!instance) return "";
        String *AssetName = instance->invoke_method<String *>("GetAssetName", sid);
        if (AssetName) return AssetName->to_string();
        return "";
    }
    int GetNameID(int sid) {
        Object *instance = get_Instance();
        if (!instance) return 0;
        return instance->invoke_method<int>("GetNameID", sid);
    }
    Item_Type GetItemType(int sid) {
        Object *instance = get_Instance();
        if (!instance) return Item_Type::End;
        return instance->invoke_method<Item_Type>("GetItemType", sid);
    }
    int GetGrade(int itemID) {
        Object *instance = get_Instance();
        if (!instance) return 0;
        return instance->invoke_method<int>("GetGrade", itemID);
    }
}

