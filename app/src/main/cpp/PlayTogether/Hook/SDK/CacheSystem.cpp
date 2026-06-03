#include "CacheSystem.h"
#include "SystemHelper.h"

namespace CacheSystem {
    Class *get_class() {
        return FindClass(OBF("CacheSystem"));
    }
    Object *get_Instance() {
        return SystemHelper::get_Cache();
    }
    Object *get_CacheUser() {
        Object *instance = get_Instance();
        if (!instance) return nullptr;
        return instance->invoke_method<Object *>(OBF("get_CacheUser"));
    }
    Object *get_CachePost() {
        Object *instance = get_Instance();
        if (!instance) return nullptr;
        return instance->invoke_method<Object *>(OBF("get_CachePost"));
    }
}
