#pragma once

#include <API/Il2CppApi.h>
#include <API/Il2cpp_Struct.h>
#include <Includes/obfuscate.h>
#include "enum/eTableType.h"

namespace TableSystem {

Class *get_class();
Object *get_Instance();
Object *GetTableUnit(eTableType type);
int GetCount(Object *tableUnit);
int GetDictCount(Object *tableUnit);
bool get_IsBuild(Object *tableUnit);
void LoadBuildToTable(Object *tableUnit);
void LoadTable(eTableType type, Object *tableUnit);
bool TryForceLoad(eTableType type, Object *tableUnit);

template<typename Fn>
void ForEachDictValue(Object *tableUnit, int cap, Fn fn) {
    if (!tableUnit || cap <= 0) return;
    Class *cls = tableUnit->get_class();
    if (!cls || !cls->find_method(OBF("get_Container"), 0)) return;
    Object *rawDict = tableUnit->invoke_method<Object *>(OBF("get_Container"));
    if (!rawDict) return;
    auto *dict = (Dictionary<uint32_t, Object *> *)rawDict;
    int dictCount = dict->get_Count();
    if (dictCount <= 0) return;
    if (dictCount > cap * 4) dictCount = cap * 4;
    Object **values = dict->CollectValues();
    if (!values) return;
    int added = 0;
    for (int i = 0; i < dictCount && added < cap; i++) {
        Object *row = values[i];
        if (!row) continue;
        if (fn(row)) added++;
    }
    delete[] values;
}

}
