#pragma once

#include "PLConfig.h"

extern bool isGameLoading;
extern PLConfig &gPLConfig;

bool SaveConfig();
bool LoadConfig();
