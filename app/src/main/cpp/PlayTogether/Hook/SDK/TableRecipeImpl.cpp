#include "TableRecipeImpl.h"
#include "TableSystem.h"
#include "enum/eTableType.h"

namespace TableRecipeImpl {

namespace {

Object *recipeTable() {
    Object *impl = TableSystem::GetTableUnit(eTableType::Recipe);
    if (!impl) return nullptr;
    if (TableSystem::GetDictCount(impl) <= 0) {
        TableSystem::TryForceLoad(eTableType::Recipe, impl);
    }
    return impl;
}

unsigned int scanRecipeDict(unsigned int key, bool matchRecipeId) {
    Object *impl = recipeTable();
    if (!impl || key == 0) return 0;
    int cap = TableSystem::GetDictCount(impl);
    if (cap <= 0) return 0;
    unsigned int found = 0;
    TableSystem::ForEachDictValue(impl, cap, [&](Object *row) -> bool {
        if (!row) return false;
        Class *cls = row->get_class();
        if (!cls || !cls->find_method(OBF("get_ItemId"), 0) || !cls->find_method(OBF("get_RecipeId"), 0)) return false;
        unsigned int rid = row->invoke_method<unsigned int>(OBF("get_RecipeId"));
        unsigned int iid = row->invoke_method<unsigned int>(OBF("get_ItemId"));
        if (matchRecipeId ? rid != key : iid != key) return false;
        found = matchRecipeId ? iid : rid;
        return true;
    });
    return found;
}

} // namespace

unsigned int FindRecipeIdByItemId(unsigned int itemId) {
    return scanRecipeDict(itemId, false);
}

unsigned int FindItemIdByRecipeId(unsigned int recipeId) {
    return scanRecipeDict(recipeId, true);
}

Object *GetRecipeForItem(unsigned int itemId) {
    if (itemId == 0) return nullptr;
    Object *impl = recipeTable();
    if (!impl) return nullptr;
    Class *cls = impl->get_class();
    if (!cls || !cls->find_method(OBF("GetRecipe"), 1)) return nullptr;
    return impl->invoke_method<Object *>(OBF("GetRecipe"), itemId);
}

unsigned int ResolveRecipeId(unsigned int itemId) {
    if (itemId == 0) return 0;
    Object *recipe = GetRecipeForItem(itemId);
    if (recipe) {
        Class *recipeCls = recipe->get_class();
        if (recipeCls && recipeCls->find_method(OBF("get_RecipeId"), 0)) {
            unsigned int rid = recipe->invoke_method<unsigned int>(OBF("get_RecipeId"));
            if (rid != 0) return rid;
        }
    }
    return FindRecipeIdByItemId(itemId);
}

}
