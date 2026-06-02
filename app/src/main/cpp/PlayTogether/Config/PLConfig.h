#pragma once

#include <API/Vector3.h>
#include <string>
#include <utility>
#include <vector>

struct PLConfig {
    struct GeneralConfig {
        bool isInfo = false;
    } general;

    struct FishingConfig {
        bool enabled = false;
        bool autoCloseReward = true;
        bool autoSellTrash = false;
        int maxSellGrade = 2;
        bool stopWhenCountOver = true;
        bool smartKeepSell = false;
        int smartKeepMinGrade = 4;
        int smartKeepMaxOwned = 3;
        bool autoEquipBait = false;
        int baitItemId = 0;
        bool smartBaitByZone = false;
        bool smartBaitAutoEffect = false;
        std::vector<std::pair<unsigned int, unsigned int>> baitZonePrefs;
        int minSellValue = 0;
        bool keepCodexFish = true;
        bool filterByShadow = false;
        bool keepShadow[7] = {true, true, true, true, true, true, true};
        bool filterByLevel = false;
        int levelMin = 0;
        int levelMax = 0;
        std::vector<unsigned int> keepLevelIds;
        std::vector<std::pair<unsigned int, std::vector<unsigned int>>> learnedLevelFish;
    } fishing;

    static int GetPlayerMapID();
    static Vector3 GetPlayerPosition();
    void Load(const std::string &content);
    std::string GetConfigContent();
};

PLConfig &GetConfig();
