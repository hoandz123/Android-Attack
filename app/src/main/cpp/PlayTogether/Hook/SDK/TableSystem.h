#pragma once

#include <API/Il2CppApi.h>
#include <API/Il2cpp_Struct.h>
#include <Includes/obfuscate.h>
#include <string>
#include "enum/eTableType.h"

namespace TableSystem {

namespace {

bool containerIsList(Object *container) {
    if (!container) return false;
    Class *cls = container->get_class();
    if (!cls) return false;
    std::string name = cls->get_name();
    return name.find(OBF("List")) != std::string::npos;
}

} // namespace

Class *get_class();
Object *get_Instance();
Object *GetTableUnit(eTableType type);
int GetCount(Object *tableUnit);
int GetDictCount(Object *tableUnit);
int GetEntryCount(Object *tableUnit);
bool get_IsBuild(Object *tableUnit);
void LoadBuildToTable(Object *tableUnit);
void LoadTable(eTableType type, Object *tableUnit);
bool TryForceLoad(eTableType type, Object *tableUnit);

template<typename Fn>
void ForEachDictValue(Object *tableUnit, int cap, Fn fn) {
    if (!tableUnit || cap <= 0) return;
    Class *cls = tableUnit->get_class();
    if (!cls || !cls->find_method(OBF("get_Container"), 0) ||
        !cls->find_method(OBF("GetTableData"), 1)) {
        return;
    }
    Object *rawContainer = tableUnit->invoke_method<Object *>(OBF("get_Container"));
    if (!rawContainer || containerIsList(rawContainer)) return;

    auto *dict = (Dictionary<uint32_t, Object *> *)rawContainer;
    int dictCount = dict->get_Count();
    if (dictCount <= 0) return;
    if (dictCount > cap * 4) dictCount = cap * 4;

    uint32_t *keys = dict->CollectKeys();
    if (!keys) return;
    int added = 0;
    for (int i = 0; i < dictCount && added < cap; i++) {
        Object *row = tableUnit->invoke_method<Object *>(OBF("GetTableData"), keys[i]);
        if (!row) continue;
        if (fn(row)) added++;
    }
    delete[] keys;
}

template<typename Fn>
void ForEachListRow(Object *tableUnit, int cap, Fn fn) {
    if (!tableUnit || cap <= 0) return;
    Class *cls = tableUnit->get_class();
    if (!cls || !cls->find_method(OBF("get_Count"), 0)) return;
    int count = tableUnit->invoke_method<int>(OBF("get_Count"));
    if (count <= 0) return;
    if (count > cap) count = cap;
    if (cls->find_method(OBF("GetTableData"), 1)) {
        for (int i = 0; i < count; i++) {
            Object *row = tableUnit->invoke_method<Object *>(OBF("GetTableData"), i);
            if (!row) continue;
            if (!fn(row)) return;
        }
        return;
    }
    if (!cls->find_method(OBF("get_Container"), 0)) return;
    Object *rawList = tableUnit->invoke_method<Object *>(OBF("get_Container"));
    if (!rawList) return;
    auto *list = (List<Object *> *) rawList;
    int n = list->get_Count();
    if (n <= 0) return;
    if (n > cap) n = cap;
    for (int i = 0; i < n; i++) {
        Object *row = list->get_item(i);
        if (!row) continue;
        if (!fn(row)) return;
    }
}

}
