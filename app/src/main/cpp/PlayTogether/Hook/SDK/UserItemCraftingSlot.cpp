#include "UserItemCraftingSlot.h"

namespace UserItemCraftingSlot {

namespace {

constexpr size_t kEndTimeOffset = 0x30;

long long dateTicks(Object *boxed) {
    if (!boxed) return 0;
    Class *cls = boxed->get_class();
    if (cls && cls->find_method(OBF("get_Ticks"), 0)) return boxed->invoke_method<long long>(OBF("get_Ticks"));
    void *raw = il2cpp_object_unbox(boxed);
    return raw ? *static_cast<long long *>(raw) : 0;
}

long long utcTicks() {
    static Class *dt = nullptr;
    static Il2CppMethod *utc = nullptr;
    if (!dt) dt = FindClass(OBF("System.DateTime"));
    if (!dt) return 0;
    if (!utc) utc = dt->get_method(OBF("get_UtcNow"), 0);
    return utc ? dateTicks(il2cpp_runtime_invoke(utc, nullptr, nullptr, nullptr)) : 0;
}

} // namespace

Class *get_class() {
    static Class *cached = nullptr;
    if (!cached) cached = FindClass(OBF("PlayTogether.UserItemCraftingSlot"));
    return cached;
}

int16_t get_SlotId(Object *slot) {
    if (!slot) return -1;
    return slot->invoke_method<int16_t>(OBF("get_SlotId"));
}

bool get_IsOpen(Object *slot) {
    if (!slot) return false;
    return slot->invoke_method<bool>(OBF("get_IsOpen"));
}

unsigned int get_RecipeId(Object *slot) {
    if (!slot) return 0;
    return slot->invoke_method<unsigned int>(OBF("get_RecipeId"));
}

unsigned int get_ItemId(Object *slot) {
    if (!slot) return 0;
    return slot->invoke_method<unsigned int>(OBF("get_ItemId"));
}

long long get_EndTimeTicks(Object *slot) {
    if (!slot) return 0;
    Class *cls = slot->get_class();
    if (cls && cls->find_method(OBF("get_EndTime"), 0)) {
        Il2CppMethod *method = cls->get_method(OBF("get_EndTime"), 0);
        if (method) {
            long long ticks = dateTicks(il2cpp_runtime_invoke(method, slot, nullptr, nullptr));
            if (ticks) return ticks;
        }
    }
    return *reinterpret_cast<long long *>(reinterpret_cast<uint8_t *>(slot) + kEndTimeOffset);
}

bool isIdle(Object *slot) {
    return get_IsOpen(slot) && get_RecipeId(slot) == 0;
}

bool isReady(Object *slot) {
    if (!get_IsOpen(slot) || get_RecipeId(slot) == 0) return false;
    long long end = get_EndTimeTicks(slot), now = utcTicks();
    return end > 0 && now > 0 && now >= end;
}

void copyFrom(Object *dst, Object *src) {
    if (!dst || !src) return;
    Class *cls = get_class();
    if (!cls || !cls->find_method(OBF("Copy"), 1)) return;
    dst->invoke_method<void>(OBF("Copy"), src);
}

}
