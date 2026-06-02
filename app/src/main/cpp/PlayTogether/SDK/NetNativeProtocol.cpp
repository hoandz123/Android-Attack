#include "NetNativeProtocol.h"
#include <Includes/Logger.h>
#include <Tools/Tools.h>
#include <API/Il2CppApi.h>
#include "enum/Item_Type.h"

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

    void SendToItemSell(void *sellItemList, Item_Type type, int targetNPC) {
        Object *instance = GetNetNativeProtocol();
        if (!instance) return;
        void *nullVal = nullptr;
        instance->invoke_method<void>("SendToItemSell", sellItemList, type, targetNPC, nullVal);
    }

    void SendToItemUse(long uid) {
        Object *instance = GetNetNativeProtocol();
        if (!instance) return;
        instance->invoke_method<void>("SendToItemUse", uid);
    }
}
