#pragma once

#include <string>

struct PLConfig {
    struct FishingConfig {
        bool enabled = false;
        bool autoCloseReward = true;
        bool autoEquipBait = false;
        int baitItemId = 0;
        bool autoCraftBait = false;
        unsigned int craftBaitItemId = 0;
        int craftBaitTargetCount = 10;
        bool fakeZoneEnabled = false;
        unsigned int fakeZoneId = 0;
        bool filterByShadow = false;
        bool keepShadow[7] = {true, true, true, true, true, true, true};
        bool filterByLevel = false;
        std::string keepLevels;
        bool sellByShadow = false;
        bool sellShadow[7] = {};
        bool sellByGrade = false;
        bool sellGrade[5] = {};
    } fishing;

    void Load(const std::string &content);
    std::string GetConfigContent();
};

extern bool isGameLoading;
extern PLConfig gPLConfig;

bool SaveConfig();
bool LoadConfig();
