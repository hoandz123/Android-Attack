#pragma once

#include <API/Il2CppApi.h>

namespace TableItemCraftingListImpl {

Object *GetCraftingInfo(unsigned int recipeId);
unsigned int GetCraftingTime(unsigned int recipeId);
bool UsesSlotCraft(unsigned int recipeId);

}
