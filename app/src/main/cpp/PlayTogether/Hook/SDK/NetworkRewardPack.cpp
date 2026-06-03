#include "NetworkRewardPack.h"
#include <API/Il2cpp_Struct.h>

namespace NetworkRewardPack {

Class *get_class() {
    static Class *cached = nullptr;
    if (!cached) cached = FindClass(OBF("PlayTogetherSocket.NetworkRewardPack"));
    return cached;
}

Object *get_RewardList(Object *pack) {
    if (!pack) return nullptr;
    return pack->invoke_method<Object *>(OBF("get_RewardList"));
}

Object *get_ItemRewardList(Object *pack) {
    if (!pack) return nullptr;
    return pack->invoke_method<Object *>(OBF("get_ItemRewardList"));
}

Object *get_LastCoinList(Object *pack) {
    if (!pack) return nullptr;
    return pack->invoke_method<Object *>(OBF("get_LastCoinList"));
}

Object *getFirstCoin(Object *pack) {
    Object *coins = get_LastCoinList(pack);
    if (!coins) return nullptr;
    auto *list = (List<Object *> *) coins;
    return list->get_Count() > 0 ? list->get_item(0) : nullptr;
}

}
