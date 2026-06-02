#ifndef SDK_NETNATIVEPROTOCOL_H
#define SDK_NETNATIVEPROTOCOL_H

#include <cstdint>
#include <API/Il2CppApi.h>

namespace NetNativeProtocol {
    Class *get_class();
    Object *GetNetNativeProtocol();
    void SendToItemRepair(long uid, Object *cb);
}

#endif // SDK_NETNATIVEPROTOCOL_H
