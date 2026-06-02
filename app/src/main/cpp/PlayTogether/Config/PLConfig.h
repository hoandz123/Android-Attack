#pragma once

#include <map>
#include <string>
#include <unordered_map>
#include <vector>
#include <API/Vector3.h>
#include "../FishLogger.h"
#include "../Stubs/AutoCollect.h"

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
        bool isTeleNpc = true;
        bool isRepair = false;
        bool isBaoQuan = false;
        bool isADS = false;
        bool isKhoaCam = false;
        bool isMoHopQua = false;
        bool isMoHopQua2 = false;
        bool isBanGoi = false;
        bool isDuNenVip = false;
        bool isDuB67 = false;
        bool isBoQuaLoiThoaiNPC = true;
        bool isTangTocGame = false;
        bool isResetTrangThai = false;
        bool isNhanThanhTich = false;
        bool isNhanNhiemVuNgay = false;
        bool isNhanTemNgay = false;
        bool isNhanThu = false;
        int chaynhanh = 0;
        int nhaycao = 0;
    } general;

    struct AutoCatchConfig {
        bool isAuto = false;
        bool isRetrieve = true;
        bool isCheck = true;
        bool isInstall = false;
        uint32_t mainItemId = 0;
        uint32_t subItemId = 0;
        int catchType = 1;
        struct ESP {
            bool isEnable = false;
        } esp;
    } autoCatch;

    struct FishingConfig {
        inline static int curFishLevel = 0;
        inline static int curFishShadowLevel = 0;
        inline static int curFishZone = 0;
        inline static FishLogger gFishLogger;

        bool isCauCa = false;
        bool isFakeVR = false;
        bool isLocID = false;
        bool isAntiLocID = false;
        int locFish = 0;
        bool isFishZone = false;
        int fishZone = 0;
        int RollCapDo = 0;
        std::map<int, bool> IDLocCa;
        std::map<int, bool> locBong;

        struct MagicWater {
            bool isEnable = false;
            int levelUses[3] = {0, 0, 0};
        } magicWater;
    } fishing;

    struct CollectConfig {
        bool isAutoDapDa = false;
        bool isAutoNguyenLieu = false;
        bool isAutoNhatThe = false;
        bool isTeleMapCollect = false;
        int delayNextMap = 10000;
        int delayDapDa = 1000;
        std::map<int, bool> DSMapDa;
        std::map<CollectSys::SpawnType, bool> DSTypeDa;

        struct ESP {
            bool isEnable = false;
            bool isShowName = false;
            bool isTeleportButton = false;
            bool isVein = false;
            bool isPlants = false;
            bool isFossil = false;
            bool isSlime = false;
            bool isSnowman = false;
            bool isOre = false;
            bool isIng = false;
            bool isFishingZone = false;
            bool isGathering = false;
            bool isCardCollect = false;
            bool isCoin = false;
            bool isNameTag = false;
            bool isFishBreadShop = false;
            bool isDragonVillageMonster = false;
        } esp;
    } collect;

    struct MonsterConfig {
        bool isEnable = false;
        bool isAutoMonster = false;
        bool isCollectReward = false;
        bool isTeleMapMonster = false;
        int delayNextMap = 10000;
        int tocDoBanQuaiVat = 800;
        int banQuaiVatHpDuoi = 100000;
        std::map<int, bool> DSMapMonster;

        struct ESP {
            bool isEnable = false;
            bool isShowName = false;
            bool isTeleportButton = false;
        } esp;
    } monster;

    struct FarmConfig {
        bool isAutoClickCollect = false;
        bool isAutoPlant = false;
        uint32_t selectedSeedId = 0;
        std::map<int, bool> selectedPlotPositions;
        int delayPlant = 1000;
        bool isAutoReap = false;
        std::map<uint32_t, bool> selectedCropTypes;
        std::map<int, bool> selectedReapPositions;
        int delayReap = 1000;
        struct ESP {
            bool isEnable = false;
            bool isShowName = false;
            bool isShowType = false;
            bool isTeleportButton = false;
        } esp;
    } farm;

    struct InsectConfig {
        inline static int TotalInsect = 0;
        inline static bool isSell = false;
        inline static std::string dateResetInsect = "";

        bool isAutoBatBo = false;
        bool isFreezeCT = false;
        bool isTeleMapBo = false;
        bool isBanBo = false;
        bool isBatBoTrenTroi = false;
        bool isNhatThe = false;
        bool isDuTimTroLen = false;
        int minInsectGrade = -1;
        int delayBatCT = 10000;
        int delayTeleMap = 10000;
        int conTrungCachMatDat = 5;
        bool FullInsect = false;
        int MaxInsectSell = 300;
        std::map<int, bool> DSNenBo;
        std::map<int, bool> DSMapBo;

        struct ESP {
            bool isEnable = false;
            bool isShowName = false;
            bool isTeleportButton = false;
            std::map<int, bool> isShowGrade;
        } esp;
    } insect;

    struct MiniGameConfig {
        struct DiggingConfig {
            bool isEnable = false;
            bool isAutoKB = false;
            bool isLocDaoKB = false;
            bool isAutoBuyXeng = false;
            float speedDaoKB = 0;
            bool isAutoDigTreasure = false;
            int capDoAnToan = 5;
            float gocLechToiDa = 0.0f;
            float chuKyChuS = 0.0f;
            float doCongChuS = 0.0f;
            float khoangCachBatDauCham = 15.0f;
            bool autoMoveKhiHetRuong = true;
            float radiusTim = 100.0f;
            float maxKhoangCach = 200.0f;
            std::map<int, bool> filterLoaiRuong;
            struct EspConfig {
                bool isEnable = false;
                bool isShowName = false;
                bool isTeleportButton = false;
            } esp;
        } digging;
        struct Zombie {
            bool isEnable = false;
            bool isChemXa = false;
            bool isEsp = false;
        } zombie;
        struct TowerClimb {
            bool isEnable = false;
            int delayNextPoint = 10000;
        } towerClimb;
        struct Obby {
            bool isEnable = false;
            int delayNextPoint = 10000;
        } obby;
        struct ThapGa {
            bool isEnable = false;
            int delayNextPoint = 10000;
        } ThapGa;
        struct Party {
            bool isEnable = false;
            int delayNextPoint = 10000;
        } Party;
    } miniGame;

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
