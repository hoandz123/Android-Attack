#pragma once

#include <map>
#include <string>
#include <unordered_map>
#include <vector>
#include <API/Vector3.h>
#include "../FishLogger.h"

struct PLConfig {
    struct NPCData {
        void *instance;
        Vector3 pos;
        std::string name;
        long uid;
    };

    inline static std::unordered_map<void *, NPCData> npcMap;

    struct NameAndPos {
        std::string name;
        int mapID = 0;
        Vector3 pos;
        bool isRandom = false;
    };
    std::map<std::string, NameAndPos> viTriCoSan;

    struct GeneralConfig {
        bool isInfo = false;
        bool isRepair = false;
        bool isBoQuaLoiThoaiNPC = true;
        bool isResetTrangThai = false;
        int chaynhanh = 0;
        int nhaycao = 0;
    } general;

    struct FishingConfig {
        inline static int curFishLevel = 0;
        inline static int curFishShadowLevel = 0;
        inline static int curFishZone = 0;
        inline static FishLogger gFishLogger;
        inline static int totalCaught = 0;
        inline static int totalSkipped = 0;
        inline static int totalFailed = 0;
        inline static std::string curStateName = "None";

        bool isCauCa = false;
        bool isFakeVR = false;
        bool isLocID = false;
        int locFish = 0;
        bool isFishZone = false;
        int fishZone = 0;
        int RollCapDo = 0;
        int delayAutoMs = 500;
        std::map<int, bool> IDLocCa;
        std::map<int, bool> locBong;

        bool isBaoQuan = false;
        bool isBanGoi = false;
        bool isDuNenVip = false;
        bool isDuB67 = false;

        struct MagicWater {
            bool isEnable = false;
            int levelUses[3] = {0, 0, 0};
        } magicWater;

        struct ESP {
            bool isEnable = false;
            bool isShowZone = false;
            bool isShowStatus = true;
        } esp;
    } fishing;

    struct MapInfo {
        std::string name;
        int id = 0;
    };

    static int GetPlayerMapID();
    static Vector3 GetPlayerPosition();
    static std::vector<MapInfo> GetMapInfoList();
    static void NextMapPos(int mapID, Vector3 pos);
    void Load(const std::string &content);
    std::string GetConfigContent();
};

PLConfig &GetConfig();
