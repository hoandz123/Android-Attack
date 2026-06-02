#include "NetNativeProtocol.h"
#include <Includes/Logger.h>
#include <Tools/Tools.h>
#include <API/Il2CppApi.h>

namespace NetNativeProtocol {
    Class *get_class() {
        return FindClass("PlayTogether.Network.Native.NetNativeProtocol");
    }

    Object *GetNetNativeProtocol() {
        Object *NetNativeProtocol;
        IL2Cpp::Il2CppGetStaticFieldValue(OBF("NetworkSystem"), OBF("NetNative"), &NetNativeProtocol);
        return NetNativeProtocol;
    }

    void SendToItemRepair(long uid, Object *cb) {
        Object *instance = GetNetNativeProtocol();
        if (!instance) return;
        instance->invoke_method<void>("SendToItemRepair", uid, cb);
    }
}
