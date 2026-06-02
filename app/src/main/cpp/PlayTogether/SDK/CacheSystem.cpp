#include "CacheSystem.h"
#include "SystemHelper.h"

namespace CacheSystem {
    Class *get_class() {
        return FindClass("CacheSystem");
    }
    Object *get_Instance() {
        return SystemHelper::get_Cache();
    }
    Object *get_CacheUser() {
        Object *instance = get_Instance();
        if (!instance) return nullptr;
        return instance->invoke_method<Object *>("get_CacheUser");
    }
}
