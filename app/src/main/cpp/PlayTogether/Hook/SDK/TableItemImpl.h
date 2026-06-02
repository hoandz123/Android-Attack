#pragma once

#include <API/Il2CppApi.h>
#include <string>

namespace TableItemImpl {

Class *get_class();
Object *get_Instance();
Object *GetTableData(unsigned int itemId);
unsigned int GetNameID(unsigned int itemId);
int GetGrade(unsigned int itemId);
int GetItemType(unsigned int itemId);
int GetSellValue(unsigned int itemId);
bool GetIsSell(unsigned int itemId);
std::string GetDisplayName(unsigned int itemId);

}
