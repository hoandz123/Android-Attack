#include "DialogShopInGame.h"
#include "Config/Config.h"
#include <Tools/Tools.h>

namespace DialogShopInGame {
    Class *get_class() {
        return FindClass("DialogShopInGame");
    }

    Object *get_Instance() {
        Object *val = nullptr;
        GET_STATIC_FIELD("DialogShopInGame", "Self", &val);
        return val;
    }

    void (*old_Update)(Object *);
    void Update(Object *instance) {
        old_Update(instance);
    }
}
