#include "TableSystem.h"
#include <API/Il2CppApi.h>
#include "enum/eTableType.h"
#include "SystemHelper.h"

namespace TableSystem {
    Class *get_class() {
        return FindClass("TableSystem");
    }
    Object *get_Instance() {
        return SystemHelper::get_Table();
    }
    Dictionary<int, Object *> *Tables() {
        Object *instance = get_Instance();
        if (!instance) return nullptr;
        return instance->get_field_object<Dictionary<int, Object *> *>("Tables");
    }
}

