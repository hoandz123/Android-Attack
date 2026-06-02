#include "TableItemImpl.h"
#include "TableSystem.h"
#include "TableMessagesImpl.h"
#include "enum/eTableType.h"

namespace TableItemImpl {

Class *get_class() {
    return FindClass(OBF("TableItemImpl"));
}

Object *get_Instance() {
    static Object *s_table = nullptr;
    static bool s_tried = false;
    if (s_tried) return s_table;
    s_tried = true;
    s_table = TableSystem::GetTableUnit(eTableType::Item);
    return s_table;
}

Object *GetTableData(unsigned int itemId) {
    if (itemId == 0) return nullptr;
    Object *itemTable = get_Instance();
    if (!itemTable) return nullptr;
    Class *cls = itemTable->get_class();
    if (!cls || !cls->find_method(OBF("GetTableData"), 1)) return nullptr;
    return itemTable->invoke_method<Object *>(OBF("GetTableData"), itemId);
}

unsigned int GetNameID(unsigned int itemId) {
    Object *itemTable = get_Instance();
    if (!itemTable || itemId == 0) return 0;
    Class *cls = itemTable->get_class();
    if (!cls || !cls->find_method(OBF("GetNameID"), 1)) return 0;
    return itemTable->invoke_method<unsigned int>(OBF("GetNameID"), itemId);
}

int GetGrade(unsigned int itemId) {
    Object *itemTable = get_Instance();
    if (!itemTable || itemId == 0) return 0;
    Class *cls = itemTable->get_class();
    if (!cls || !cls->find_method(OBF("GetGrade"), 1)) return 0;
    return itemTable->invoke_method<int>(OBF("GetGrade"), itemId);
}

int GetItemType(unsigned int itemId) {
    Object *itemTable = get_Instance();
    if (!itemTable || itemId == 0) return 0;
    Class *cls = itemTable->get_class();
    if (!cls || !cls->find_method(OBF("GetItemType"), 1)) return 0;
    return (int) itemTable->invoke_method<int>(OBF("GetItemType"), itemId);
}

int GetSellValue(unsigned int itemId) {
    Object *item = GetTableData(itemId);
    if (!item) return -1;
    Class *cls = item->get_class();
    if (!cls || !cls->find_method(OBF("get_SellValue"), 0)) return -1;
    return item->invoke_method<int>(OBF("get_SellValue"));
}

bool GetIsSell(unsigned int itemId) {
    Object *item = GetTableData(itemId);
    if (!item) return false;
    Class *cls = item->get_class();
    if (!cls || !cls->find_method(OBF("get_IsSell"), 0)) return false;
    return item->invoke_method<bool>(OBF("get_IsSell"));
}

std::string GetDisplayName(unsigned int itemId) {
    if (itemId == 0) return {};
    std::string name = TableMessagesImpl::GetMessageDx(GetNameID(itemId));
    if (!name.empty()) return name;
    char buf[48];
    snprintf(buf, sizeof(buf), OBF("Item %u"), itemId);
    return std::string(buf);
}

}
