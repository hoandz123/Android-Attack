#ifndef SDK_NETNATIVEPROTOCOL_H
#define SDK_NETNATIVEPROTOCOL_H

#include <cstdint>
#include <API/Il2CppApi.h>
#include "enum/Item_Type.h"

namespace NetNativeProtocol {
    Class *get_class();
    Object *GetNetNativeProtocol();
    void SendToItemUse(long uid);
    void SendToItemRepair(long uid, Object *cb);
    void SendToItemSell(void *sellItemList, Item_Type type, int targetNPC);
}

#endif // SDK_NETNATIVEPROTOCOL_H
