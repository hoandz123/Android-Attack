#include "ItemCraftingRewardItemA.h"

namespace ItemCraftingRewardItemA {

Class *get_class() {
    static Class *cached = nullptr;
    if (!cached) cached = FindClass(OBF("PlayTogetherSocket.ItemCraftingRewardItemA"));
    return cached;
}

Object *get_Reward(Object *protocol) {
    if (!protocol) return nullptr;
    return protocol->invoke_method<Object *>(OBF("get_Reward"));
}

Object *get_Slot(Object *protocol) {
    if (!protocol) return nullptr;
    return protocol->invoke_method<Object *>(OBF("get_Slot"));
}

}
