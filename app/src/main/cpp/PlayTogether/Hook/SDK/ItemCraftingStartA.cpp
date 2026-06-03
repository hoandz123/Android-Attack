#include "ItemCraftingStartA.h"

namespace ItemCraftingStartA {

Class *get_class() {
    static Class *cached = nullptr;
    if (!cached) cached = FindClass(OBF("PlayTogetherSocket.ItemCraftingStartA"));
    return cached;
}

Object *get_UserItemCraftingSlot(Object *protocol) {
    if (!protocol) return nullptr;
    return protocol->invoke_method<Object *>(OBF("get_UserItemCraftingSlot"));
}

Object *get_UpdatedItems(Object *protocol) {
    if (!protocol) return nullptr;
    return protocol->invoke_method<Object *>(OBF("get_UpdatedItems"));
}

Object *get_UserCoin(Object *protocol) {
    if (!protocol) return nullptr;
    return protocol->invoke_method<Object *>(OBF("get_UserCoin"));
}

}
