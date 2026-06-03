#ifndef SDK_USERITEMCRAFTINGSLOT_H
#define SDK_USERITEMCRAFTINGSLOT_H

#include <API/Il2CppApi.h>
#include <cstdint>

namespace UserItemCraftingSlot {

Class *get_class();
int16_t get_SlotId(Object *slot);
bool get_IsOpen(Object *slot);
unsigned int get_RecipeId(Object *slot);
unsigned int get_ItemId(Object *slot);
long long get_EndTimeTicks(Object *slot);
bool isIdle(Object *slot);
bool isReady(Object *slot);
void copyFrom(Object *dst, Object *src);

}

#endif // SDK_USERITEMCRAFTINGSLOT_H
