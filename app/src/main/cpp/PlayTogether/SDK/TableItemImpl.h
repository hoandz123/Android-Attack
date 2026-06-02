#ifndef SDK_TABLEITEMIMPL_H
#define SDK_TABLEITEMIMPL_H

#include <API/Il2CppApi.h>
#include "enum/Item_Type.h"

namespace TableItemImpl {
    Class *get_class();
    Object *get_Instance();
    Object *GetTableData(int sid);
    std::string GetAssetName(int sid);
    int GetNameID(int sid);
    Item_Type GetItemType(int sid);
    int GetGrade(int itemID);
}

#endif // SDK_TABLEITEMIMPL_H

