#include "TableIngredientImpl.h"
#include "TableSystem.h"
#include "enum/eTableType.h"

namespace TableIngredientImpl {

Class *get_class() {
    static Class *cached = nullptr;
    if (!cached) cached = FindClass(OBF("TableIngredientImpl"));
    return cached;
}

unsigned int GetProductId(unsigned int ingredientItemId) {
    if (!ingredientItemId) return 0;
    Object *impl = TableSystem::GetTableUnit(eTableType::Ingredient);
    if (!impl) return 0;
    Class *cls = impl->get_class();
    if (!cls || !cls->find_method(OBF("GetTableData"), 1)) return 0;
    if (TableSystem::GetDictCount(impl) <= 0) {
        TableSystem::TryForceLoad(eTableType::Ingredient, impl);
    }
    Object *row = impl->invoke_method<Object *>(OBF("GetTableData"), ingredientItemId);
    if (!row) return 0;
    Class *rowCls = row->get_class();
    if (!rowCls || !rowCls->find_method(OBF("get_ProductId"), 0)) return 0;
    return row->invoke_method<unsigned int>(OBF("get_ProductId"));
}

}
