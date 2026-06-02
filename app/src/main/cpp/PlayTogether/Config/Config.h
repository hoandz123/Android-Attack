#pragma once

#include <string>
#include <utility>
#include <vector>

struct PLConfig {
    struct FishingConfig {
        bool enabled = false;
        bool autoCloseReward = true;
        bool autoSellTrash = false;
        int maxSellGrade = 2;
        bool stopWhenCountOver = true;
        bool autoEquipBait = false;
        int baitItemId = 0;
        bool smartBaitByZone = false;
        bool smartBaitAutoEffect = false;
        std::vector<std::pair<unsigned int, unsigned int>> baitZonePrefs;
        bool filterByShadow = false;
        bool keepShadow[7] = {true, true, true, true, true, true, true};
        bool filterByLevel = false;
        std::string keepLevels;
    } fishing;

    void Load(const std::string &content);
    std::string GetConfigContent();
};

extern bool isGameLoading;
extern PLConfig gPLConfig;

bool SaveConfig();
bool LoadConfig();
