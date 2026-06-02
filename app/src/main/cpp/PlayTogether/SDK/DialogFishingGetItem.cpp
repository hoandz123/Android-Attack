#include "DialogFishingGetItem.h"
#include <API/Il2CppApi.h>
#include "Config/Config.h"

namespace DialogFishingGetItem {
    Class *get_class() {
        return FindClass("DialogFishingGetItem");
    }
    void (*old_OnItemSell)(Object *thiz, bool success);
    void OnItemSell(Object *thiz, bool success) {
        if (gPLConfig.fishing.isBanGoi) {
            return;
        }
        old_OnItemSell(thiz, success);
    }
}

