#pragma once

#include <API/Il2CppApi.h>

namespace TableRecipeImpl {

unsigned int FindRecipeIdByItemId(unsigned int itemId);
unsigned int FindItemIdByRecipeId(unsigned int recipeId);
Object *GetRecipeForItem(unsigned int itemId);
unsigned int ResolveRecipeId(unsigned int itemId);

}
