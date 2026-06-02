#include "TableFishingAreaImpl.h"
#include "TableSystem.h"
#include "enum/eTableType.h"

namespace TableFishingAreaImpl {

Class *get_class() {
    return FindClass(OBF("TableFishingAreaImpl"));
}

Object *get_Instance() {
    return TableSystem::GetTableUnit(eTableType::FishingArea);
}

Object *GetTableData(unsigned int zoneId) {
    if (zoneId == 0) return nullptr;
    Object *impl = get_Instance();
    if (!impl) return nullptr;
    Class *cls = impl->get_class();
    if (!cls || !cls->find_method(OBF("GetTableData"), 1)) return nullptr;
    return impl->invoke_method<Object *>(OBF("GetTableData"), zoneId);
}

unsigned int GetActionId(unsigned int zoneId) {
    Object *area = GetTableData(zoneId);
    if (!area) return 0;
    Class *cls = area->get_class();
    if (!cls || !cls->find_method(OBF("get_ActionId"), 0)) return 0;
    return area->invoke_method<unsigned int>(OBF("get_ActionId"));
}

unsigned int GetZoneTextId(unsigned int zoneId) {
    Object *area = GetTableData(zoneId);
    if (!area) return 0;
    Class *cls = area->get_class();
    if (!cls || !cls->find_method(OBF("get_FishingZoneText"), 0)) return 0;
    return area->invoke_method<unsigned int>(OBF("get_FishingZoneText"));
}

}
