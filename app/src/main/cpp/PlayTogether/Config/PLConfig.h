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
        int hitDelayMs = 0;
        int liftDelayMs = 0;
        bool autoSellTrash = false;
        int maxSellGrade = 2;
        bool pauseOnRareCatch = false;
        int minRareGrade = 4;
        bool showSessionStats = true;
        bool showFloatMarker = false;
        bool showZoneInfo = true;
        bool showFailHint = true;
        bool showEfficiency = true;
        bool showBigFishHp = true;
        bool adaptiveCastBackoff = true;
        bool stopWhenCountOver = true;
        bool skipBoastDelay = false;
        int boastSkipMs = 400;
        bool autoPerfectTug = true;
        int perfectLiftIntervalMs = 180;
        bool fastBite = false;
        bool smartKeepSell = false;
        int smartKeepMinGrade = 4;
        int smartKeepMaxOwned = 3;
        bool autoEquipBait = false;
        int baitItemId = 0;
        int targetFishItemId = 0;
        bool adaptivePacing = true;
        bool stunOrchestrator = true;
        int stunHitIntervalMs = 220;
        int maxStunHitsPerPhase = 8;
        int minSellValue = 0;
        bool keepCodexFish = true;
        bool autoRaidEnter = false;
    } fishing;

    static int GetPlayerMapID();
    static Vector3 GetPlayerPosition();
    void Load(const std::string &content);
    std::string GetConfigContent();
};

PLConfig &GetConfig();
