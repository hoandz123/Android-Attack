#ifndef SDK_EQUIPMENTSYSTEM_H
#define SDK_EQUIPMENTSYSTEM_H

#include <API/Il2CppApi.h>

namespace EquipmentSystem {
    Class *get_class();
    extern void (*old_ShowRepairItem)(Object *thiz, int64_t uid, Object *cb);
    void ShowRepairItem(Object *thiz, int64_t uid, Object *cb);
}

#endif // SDK_EQUIPMENTSYSTEM_H

