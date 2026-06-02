#include "TableSystem.h"
#include "SystemHelper.h"
#define LOGGER_TAG "ATTACK_TableSystem"
#include <Includes/Logger.h>
#include <Tools/Tools.h>

namespace TableSystem {

Class *get_class() {
    return FindClass(OBF("TableSystem"));
}

Object *get_Instance() {
    return SystemHelper::get_Table();
}

Object *GetTableUnit(eTableType type) {
    Object *tableSys = get_Instance();
    if (!tableSys) return nullptr;
    Object *tables = tableSys->get_field_object<Object *>(OBF("Tables"));
    if (!tables) return nullptr;
    Class *tablesCls = tables->get_class();
    if (!tablesCls || !tablesCls->find_method(OBF("get_Item"), 1)) return nullptr;
    int key = (int) type;
    return tables->invoke_method<Object *>(OBF("get_Item"), key);
}

int GetCount(Object *tableUnit) {
    if (!tableUnit) return -1;
    Class *cls = tableUnit->get_class();
    if (!cls || !cls->find_method(OBF("get_Count"), 0)) return -2;
    return tableUnit->invoke_method<int>(OBF("get_Count"));
}

int GetDictCount(Object *tableUnit) {
    if (!tableUnit) return -1;
    Class *cls = tableUnit->get_class();
    if (!cls || !cls->find_method(OBF("get_Container"), 0)) return -2;
    Object *rawDict = tableUnit->invoke_method<Object *>(OBF("get_Container"));
    if (!rawDict) return -3;
    auto *dict = (Dictionary<uint32_t, Object *> *)rawDict;
    return dict->get_Count();
}

bool get_IsBuild(Object *tableUnit) {
    if (!tableUnit) return false;
    Class *cls = tableUnit->get_class();
    if (!cls || !cls->find_method(OBF("get_IsBuild"), 0)) return false;
    return tableUnit->invoke_method<bool>(OBF("get_IsBuild"));
}

void LoadBuildToTable(Object *tableUnit) {
    if (!tableUnit) return;
    Class *cls = tableUnit->get_class();
    if (!cls || !cls->find_method(OBF("LoadBuildToTable"), 0)) return;
    tableUnit->invoke_method<void>(OBF("LoadBuildToTable"));
}

void LoadTable(eTableType type, Object *tableUnit) {
    Object *tableSys = get_Instance();
    if (!tableSys || !tableUnit) return;
    Class *cls = tableSys->get_class();
    if (!cls || !cls->find_method(OBF("LoadTable"), 2)) return;
    int key = (int) type;
    tableSys->invoke_method<void>(OBF("LoadTable"), key, tableUnit);
}

bool TryForceLoad(eTableType type, Object *tableUnit) {
    static long long s_lastTryMs = 0;
    long long now = Tools::getSystemMilliseconds();
    if (now - s_lastTryMs < 7000) return false;
    if (!tableUnit) return false;
    if (GetDictCount(tableUnit) > 0) return true;
    s_lastTryMs = now;
    LOGI(OBF("TableSystem: force-load type=%d isBuild=%d"), (int) type, get_IsBuild(tableUnit) ? 1 : 0);
    LoadBuildToTable(tableUnit);
    if (GetDictCount(tableUnit) > 0) {
        LOGI(OBF("TableSystem: force-load OK LoadBuildToTable type=%d"), (int) type);
        return true;
    }
    LoadTable(type, tableUnit);
    if (GetDictCount(tableUnit) > 0) {
        LOGI(OBF("TableSystem: force-load OK LoadTable type=%d"), (int) type);
        return true;
    }
    LOGW(OBF("TableSystem: force-load thất bại type=%d"), (int) type);
    return false;
}

}
