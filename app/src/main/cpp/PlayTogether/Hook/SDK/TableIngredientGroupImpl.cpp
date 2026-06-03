#include "TableIngredientGroupImpl.h"
#include "TableSystem.h"
#include "enum/eTableType.h"

namespace TableIngredientGroupImpl {

Class *get_class() {
    static Class *cached = nullptr;
    if (!cached) cached = FindClass(OBF("TableIngredientGroupImpl"));
    return cached;
}

unsigned int GetIngredientGroupFirstItemId(unsigned int groupId) {
    if (!groupId) return 0;
    Object *impl = TableSystem::GetTableUnit(eTableType::IngredientGroup);
    Class *cls = get_class();
    if (!impl || !cls || !cls->find_method(OBF("GetIngredientGroupFirstItemId"), 1)) return 0;
    return impl->invoke_method<unsigned int>(OBF("GetIngredientGroupFirstItemId"), groupId);
}

}
