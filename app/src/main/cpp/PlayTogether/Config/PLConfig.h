#pragma once

#include <API/Vector3.h>
#include <string>

struct PLConfig {
    struct GeneralConfig {
        bool isInfo = false;
    } general;

    struct FishingConfig {
        bool enabled = false;
        bool autoCloseReward = true;
        bool showStatus = true;
        bool handleBigFish = false;
        int tickIntervalMs = 400;
        int actionIntervalMs = 500;
        int restartDelayMs = 1500;
    } fishing;

    static int GetPlayerMapID();
    static Vector3 GetPlayerPosition();
    void Load(const std::string &content);
    std::string GetConfigContent();
};

PLConfig &GetConfig();
