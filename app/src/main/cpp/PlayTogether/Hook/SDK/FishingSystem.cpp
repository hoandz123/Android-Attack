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
