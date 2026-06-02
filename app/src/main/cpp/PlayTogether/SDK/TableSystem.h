#ifndef SDK_TABLESYSTEM_H
#define SDK_TABLESYSTEM_H

#include <API/Il2CppApi.h>
#include "enum/eTableType.h"

namespace TableSystem {
    Class *get_class();
    Object *get_Instance();
    Dictionary<int, Object *> *Tables();
    template<typename T>
    T GetTableUnit(eTableType type) {
        Dictionary<int, Object *> *tables = Tables();
        if (!tables || tables->get_Count() < 1) return nullptr;
        return tables->get_Item((int) type);
    }
    template<typename T>
    T GetTableUnit(Class* clazz) {
        Dictionary<int, Object *> *tables = Tables();
        if (!tables || tables->get_Count() < 1) return nullptr;
        for (int i = 0; i < tables->get_Count(); i++) {
            Object* table = tables->get_Item(i);
            if (table && table->get_class() == clazz) {
                return (T) table;
            }
        }
        return nullptr;
    }
}

#endif // SDK_TABLESYSTEM_H

