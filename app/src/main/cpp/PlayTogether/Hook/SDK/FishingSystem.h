#pragma once

#include <API/Il2CppApi.h>

namespace FishingSystem {

Class *get_class();
Object *get_Instance();
long long get_FishingBaitUID(Object *self);
void set_FishingBaitUID(Object *self, long long uid);
unsigned int get_CastingFishingZoneID(Object *self);
unsigned int get_CatchFishingZone(Object *self);
int get_CatchItemGrade(Object *self);

}
