#include "TableItemCraftingListImpl.h"
#include "TableSystem.h"
#include "enum/eTableType.h"

namespace TableItemCraftingListImpl {

namespace {

Object *craftingTable() {
    Object *impl = TableSystem::GetTableUnit(eTableType::ItemCraftingList);
    if (!impl) return nullptr;
    if (TableSystem::GetEntryCount(impl) <= 0) {
        TableSystem::TryForceLoad(eTableType::ItemCraftingList, impl);
    }
    return impl;
}

Object *scanByRecipeId(Object *impl, unsigned int recipeId) {
    if (!impl || !recipeId) return nullptr;
    int cap = TableSystem::GetEntryCount(impl);
    if (cap <= 0) return nullptr;
    Object *found = nullptr;
    TableSystem::ForEachListRow(impl, cap, [&](Object *row) -> bool {
        if (!row) return false;
        Class *cls = row->get_class();
        if (!cls || !cls->find_method(OBF("get_RecipeId"), 0)) return false;
        if (row->invoke_method<unsigned int>(OBF("get_RecipeId")) != recipeId) return false;
        found = row;
        return true;
    });
    return found;
}

} // namespace

Object *GetCraftingInfo(unsigned int recipeId) {
    if (!recipeId) return nullptr;
    Object *impl = craftingTable();
    if (!impl) return nullptr;
    Class *cls = impl->get_class();
    if (cls && cls->find_method(OBF("GetCraftingInfo"), 1)) {
        Object *info = impl->invoke_method<Object *>(OBF("GetCraftingInfo"), recipeId);
        if (info) return info;
    }
    return scanByRecipeId(impl, recipeId);
}

unsigned int GetCraftingTime(unsigned int recipeId) {
    Object *info = GetCraftingInfo(recipeId);
    if (!info) return 0;
    Class *cls = info->get_class();
    if (!cls || !cls->find_method(OBF("get_CraftingTime"), 0)) return 0;
    return info->invoke_method<unsigned int>(OBF("get_CraftingTime"));
}

bool UsesSlotCraft(unsigned int recipeId) {
    return GetCraftingTime(recipeId) > 0;
}

}
