#include "ItemCraftingTimeDecreaseA.h"

namespace ItemCraftingTimeDecreaseA {

Class *get_class() {
    static Class *cached = nullptr;
    if (!cached) cached = FindClass(OBF("PlayTogetherSocket.ItemCraftingTimeDecreaseA"));
    return cached;
}

Object *get_UserItemCraftingSlot(Object *protocol) {
    if (!protocol) return nullptr;
    return protocol->invoke_method<Object *>(OBF("get_UserItemCraftingSlot"));
}

Object *get_UserCoin(Object *protocol) {
    if (!protocol) return nullptr;
    return protocol->invoke_method<Object *>(OBF("get_UserCoin"));
}

Object *get_UserCount(Object *protocol) {
    if (!protocol) return nullptr;
    return protocol->invoke_method<Object *>(OBF("get_UserCount"));
}

}
