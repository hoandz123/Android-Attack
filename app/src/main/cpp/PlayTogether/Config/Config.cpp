#include "Config.h"

bool isGameLoading = false;
PLConfig &gPLConfig = GetConfig();

static PLConfig g_config;

PLConfig &GetConfig() {
    return g_config;
}
