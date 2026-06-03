#include "FishingSystem.h"
#include "SystemHelper.h"

namespace FishingSystem {

Class *get_class() {
    return FindClass(OBF("FishingSystem"));
}

Object *get_Instance() {
    return SystemHelper::get_Fishing();
}

long long get_FishingBaitUID(Object *self) {
    if (!self) return 0;
    return self->invoke_method<long long>(OBF("get_FishingBaitUID"));
}

void set_FishingBaitUID(Object *self, long long uid) {
    if (!self) return;
    self->invoke_method<void>(OBF("set_FishingBaitUID"), uid);
}

void set_CastingFishingZoneID(Object *self, unsigned int zoneId) {
    if (!self) return;
    self->set_field_value<unsigned int>(OBF("CastingFishingZoneID"), zoneId);
}

void notifyUpdateFishingBait(Object *self) {
    if (!self) return;
    Object *action = self->get_field_object<Object *>(OBF("UpdateFishingBait"));
    if (!action) return;
    Class *cls = action->get_class();
    if (!cls || !cls->find_method(OBF("Invoke"), 0)) return;
    action->invoke_method<void>(OBF("Invoke"));
}

unsigned int get_CastingFishingZoneID(Object *self) {
    if (!self) return 0;
    return self->get_field_value<unsigned int>(OBF("CastingFishingZoneID"));
}

unsigned int get_CatchFishingZone(Object *self) {
    if (!self) return 0;
    return self->invoke_method<unsigned int>(OBF("get_CatchFishingZone"));
}

int get_CatchItemGrade(Object *self) {
    if (!self) return 0;
    return (int) self->invoke_method<int>(OBF("get_CatchItemGrade"));
}

}
