#include "Config.h"
#include "SDK/CacheUser.h"

bool isGameLoading = false;

int PLConfig::GetPlayerMapID() {
    return CacheUser::myCurrentMapID();
}
PLConfig &gPLConfig = GetConfig();

static PLConfig g_config;

PLConfig &GetConfig() {
    return g_config;
}
