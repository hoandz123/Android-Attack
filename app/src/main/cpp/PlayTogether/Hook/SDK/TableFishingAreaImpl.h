#pragma once

#include <API/Il2CppApi.h>

namespace TableFishingAreaImpl {

Class *get_class();
Object *get_Instance();
Object *GetTableData(unsigned int zoneId);
unsigned int GetActionId(unsigned int zoneId);
unsigned int GetZoneTextId(unsigned int zoneId);

}
