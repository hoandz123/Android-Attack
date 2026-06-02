#include "CacheUser.h"
#include "CacheSystem.h"
#include <Includes/obfuscate.h>
#define LOG_TAG OBF("ATTACK_PlayTogether")
#include <Includes/Logger.h>

namespace CacheUser {
    Class *get_class() {
        return FindClass("CacheUser");
    }
    Object *get_Instance() {
        return CacheSystem::get_CacheUser();
    }
    int myCurrentMapID() {
        Object *instance = get_Instance();
        if (!instance) return 0;
        return instance->get_field_value<int>("myCurrentMapID");
    }
}
