#include "Config.h"
#include "SDK/CacheUser.h"
#include "SDK/ActorControl.h"
#include <Includes/obfuscate.h>

bool isGameLoading = false;

int PLConfig::GetPlayerMapID() {
    return CacheUser::myCurrentMapID();
}

Vector3 PLConfig::GetPlayerPosition() {
    if (!ActorControl::my_Motor) return Vector3();
    return ActorControl::my_Motor->invoke_method<Vector3>("get_TransientPosition");
}

PLConfig &gPLConfig = GetConfig();

static PLConfig g_config;

PLConfig &GetConfig() {
    return g_config;
}
