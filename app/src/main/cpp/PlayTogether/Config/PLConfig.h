#pragma once

#include <map>
#include <string>
#include <unordered_map>
#include <vector>
#include <API/Vector3.h>

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
        bool enabled = false;
        bool autoCloseReward = true;
        bool showStatus = true;
        bool handleBigFish = false;
        int tickIntervalMs = 400;
        int actionIntervalMs = 500;
        int restartDelayMs = 1500;
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
