//
// Created by TEAMHMG on 14/09/2025.
//

#include "DialogShopInGame.h"
#include "Config/Config.h"
#include <Tools/Tools.h>

namespace DialogShopInGame {
    Class* get_class() {
        return FindClass("DialogShopInGame");
    }
    Object *get_Instance() {
        Object* val = nullptr;
        GET_STATIC_FIELD("DialogShopInGame", "Self", &val);
        return val;
    }

    static bool valTrue = true;

    void (*old_Update)(Object *);
    void Update(Object *instance) {
        old_Update(instance);
        if (!instance) return;
        if (!gPLConfig.miniGame.digging.isAutoBuyXeng) return;
        RATE_LIMIT(500);
        instance->invoke_method<void>("set_EquipRefreshAll", valTrue);
        instance->invoke_method<void>("OnClick_ItemBuy1");
    }

}