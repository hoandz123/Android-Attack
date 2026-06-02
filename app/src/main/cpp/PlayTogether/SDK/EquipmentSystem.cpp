#include "PlayLog.h"
#include "EquipmentSystem.h"
#include <API/Il2CppApi.h>
#include "Config/Config.h"
#include "NetNativeProtocol.h"

namespace EquipmentSystem {
    Class *get_class() {
        return FindClass("EquipmentSystem");
    }
    void (*old_ShowRepairItem)(Object *thiz, int64_t uid, Object *cb);
    void ShowRepairItem(Object *thiz, int64_t uid, Object *cb) { //Sửa đồ
        LOGD("ShowRepairItem: uid=%ld, cb=%p", uid, cb);
        if (gPLConfig.general.isRepair) {
            NetNativeProtocol::SendToItemRepair(uid, cb);
            return;
        }
        old_ShowRepairItem(thiz, uid, cb);
    }
}

