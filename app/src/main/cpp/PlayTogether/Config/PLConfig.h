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
        int chaynhanh = 0;
        int nhaycao = 0;
    } general;

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
            bool isAutoBuyXeng = false;
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
        struct ThapGa {
            bool isEnable = false;
            int delayNextPoint = 10000;
        } ThapGa;
    } miniGame;

    static void NextMapPos(int mapID, Vector3 pos);
};

PLConfig &GetConfig();
