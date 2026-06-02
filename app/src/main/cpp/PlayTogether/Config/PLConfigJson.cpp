#include "PLConfig.h"
#include "Config.h"
#include "Stubs/AutoCollect.h"
#include <json/json.hpp>
#include <FileManager.hpp>
#include <Tools/Tools.h>
#include <Includes/obfuscate.h>
#define LOG_TAG OBF("ATTACK_PlayTogether")
#include <Includes/Logger.h>
#include <API/Vector3.h>

void to_json(nlohmann::json& j, const Vector3& v) {
    j = nlohmann::json{{"x", v.x}, {"y", v.y}, {"z", v.z}};
}

void from_json(const nlohmann::json& j, Vector3& v) {
    if (j.contains("x")) v.x = j["x"];
    if (j.contains("y")) v.y = j["y"];
    if (j.contains("z")) v.z = j["z"];
}

void to_json(nlohmann::json& j, const PLConfig::NameAndPos& obj) {
    j = nlohmann::json{
        {"name", obj.name},
        {"mapID", obj.mapID},
        {"pos", obj.pos},
        {"isRandom", obj.isRandom}
    };
}

void from_json(const nlohmann::json& j, PLConfig::NameAndPos& obj) {
    if (j.contains("name")) j["name"].get_to(obj.name);
    if (j.contains("mapID")) j["mapID"].get_to(obj.mapID);
    if (j.contains("pos")) j["pos"].get_to(obj.pos);
    if (j.contains("isRandom")) j["isRandom"].get_to(obj.isRandom);
}

void to_json(nlohmann::json& j, const PLConfig::GeneralConfig& cfg) {
    j = nlohmann::json{
        {"isInfo", cfg.isInfo},
        {"isTeleNpc", cfg.isTeleNpc},
        {"isRepair", cfg.isRepair},
        {"isBaoQuan", cfg.isBaoQuan},
        {"isADS", cfg.isADS},
        {"isKhoaCam", cfg.isKhoaCam},
        {"isMoHopQua", cfg.isMoHopQua},
        {"isMoHopQua2", cfg.isMoHopQua2},
        {"isBanGoi", cfg.isBanGoi},
        {"isDuNenVip", cfg.isDuNenVip},
        {"isDuB67", cfg.isDuB67},
        {"isBoQuaLoiThoaiNPC", cfg.isBoQuaLoiThoaiNPC},
        {"isTangTocGame", cfg.isTangTocGame},
        {"isResetTrangThai", cfg.isResetTrangThai},
        {"isNhanThanhTich", cfg.isNhanThanhTich},
        {"isNhanNhiemVuNgay", cfg.isNhanNhiemVuNgay},
        {"isNhanTemNgay", cfg.isNhanTemNgay},
        {"isNhanThu", cfg.isNhanThu},
        {"chaynhanh", cfg.chaynhanh},
        {"nhaycao", cfg.nhaycao}
    };
}

void from_json(const nlohmann::json& j, PLConfig::GeneralConfig& cfg) {
    if (j.contains("isInfo")) j["isInfo"].get_to(cfg.isInfo);
    if (j.contains("isTeleNpc")) j["isTeleNpc"].get_to(cfg.isTeleNpc);
    if (j.contains("isRepair")) j["isRepair"].get_to(cfg.isRepair);
    if (j.contains("isBaoQuan")) j["isBaoQuan"].get_to(cfg.isBaoQuan);
    if (j.contains("isADS")) j["isADS"].get_to(cfg.isADS);
    if (j.contains("isKhoaCam")) j["isKhoaCam"].get_to(cfg.isKhoaCam);
    if (j.contains("isMoHopQua")) j["isMoHopQua"].get_to(cfg.isMoHopQua);
    if (j.contains("isMoHopQua2")) j["isMoHopQua2"].get_to(cfg.isMoHopQua2);
    if (j.contains("isBanGoi")) j["isBanGoi"].get_to(cfg.isBanGoi);
    if (j.contains("isDuNenVip")) j["isDuNenVip"].get_to(cfg.isDuNenVip);
    if (j.contains("isDuB67")) j["isDuB67"].get_to(cfg.isDuB67);
    if (j.contains("isBoQuaLoiThoaiNPC")) j["isBoQuaLoiThoaiNPC"].get_to(cfg.isBoQuaLoiThoaiNPC);
    if (j.contains("isTangTocGame")) j["isTangTocGame"].get_to(cfg.isTangTocGame);
    if (j.contains("isResetTrangThai")) j["isResetTrangThai"].get_to(cfg.isResetTrangThai);
    if (j.contains("isNhanThanhTich")) j["isNhanThanhTich"].get_to(cfg.isNhanThanhTich);
    if (j.contains("isNhanNhiemVuNgay")) j["isNhanNhiemVuNgay"].get_to(cfg.isNhanNhiemVuNgay);
    if (j.contains("isNhanTemNgay")) j["isNhanTemNgay"].get_to(cfg.isNhanTemNgay);
    if (j.contains("isNhanThu")) j["isNhanThu"].get_to(cfg.isNhanThu);
    if (j.contains("chaynhanh")) j["chaynhanh"].get_to(cfg.chaynhanh);
    if (j.contains("nhaycao")) j["nhaycao"].get_to(cfg.nhaycao);
}

void to_json(nlohmann::json& j, const PLConfig::AutoCatchConfig::ESP& esp) {
    j = nlohmann::json{{"isEnable", esp.isEnable}};
}

void from_json(const nlohmann::json& j, PLConfig::AutoCatchConfig::ESP& esp) {
    if (j.contains("isEnable")) j["isEnable"].get_to(esp.isEnable);
}

void to_json(nlohmann::json& j, const PLConfig::AutoCatchConfig& cfg) {
    j = nlohmann::json{
        {"isAuto", cfg.isAuto},
        {"isRetrieve", cfg.isRetrieve},
        {"isCheck", cfg.isCheck},
        {"isInstall", cfg.isInstall},
        {"mainItemId", cfg.mainItemId},
        {"subItemId", cfg.subItemId},
        {"catchType", cfg.catchType},
        {"esp", cfg.esp}
    };
}

void from_json(const nlohmann::json& j, PLConfig::AutoCatchConfig& cfg) {
    if (j.contains("isAuto")) j["isAuto"].get_to(cfg.isAuto);
    if (j.contains("isRetrieve")) j["isRetrieve"].get_to(cfg.isRetrieve);
    if (j.contains("isCheck")) j["isCheck"].get_to(cfg.isCheck);
    if (j.contains("isInstall")) j["isInstall"].get_to(cfg.isInstall);
    if (j.contains("mainItemId")) j["mainItemId"].get_to(cfg.mainItemId);
    if (j.contains("subItemId")) j["subItemId"].get_to(cfg.subItemId);
    if (j.contains("catchType")) j["catchType"].get_to(cfg.catchType);
    if (j.contains("esp")) j["esp"].get_to(cfg.esp);
}

void to_json(nlohmann::json& j, const PLConfig::FishingConfig::MagicWater& mw) {
    j = nlohmann::json{
        {"isEnable", mw.isEnable},
        {"levelUses", {mw.levelUses[0], mw.levelUses[1], mw.levelUses[2]}}
    };
}

void from_json(const nlohmann::json& j, PLConfig::FishingConfig::MagicWater& mw) {
    if (j.contains("isEnable")) j["isEnable"].get_to(mw.isEnable);
    // Hỗ trợ cấu hình mới (levelUses)
    if (j.contains("levelUses")) {
        auto arr = j["levelUses"];
        for (int i = 0; i < 3; ++i) {
            if (i < (int)arr.size()) {
                arr[i].get_to(mw.levelUses[i]);
            } else {
                mw.levelUses[i] = 0;
            }
        }
    } else {
        // Backward compatible với cấu hình cũ (level + maxUses)
        int oldLevel = 0;
        int oldMaxUses = 0;
        if (j.contains("level")) j["level"].get_to(oldLevel);
        if (j.contains("maxUses")) j["maxUses"].get_to(oldMaxUses);
        mw.levelUses[0] = mw.levelUses[1] = mw.levelUses[2] = 0;
        if (oldLevel >= 0 && oldLevel < 3 && oldMaxUses > 0) {
            mw.levelUses[oldLevel] = oldMaxUses;
        }
    }
}

void to_json(nlohmann::json& j, const PLConfig::FishingConfig& cfg) {
    j = nlohmann::json{
        {"isCauCa", cfg.isCauCa},
        {"isFakeVR", cfg.isFakeVR},
        {"isLocID", cfg.isLocID},
        {"isAntiLocID", cfg.isAntiLocID},
        {"locFish", cfg.locFish},
        {"isFishZone", cfg.isFishZone},
        {"fishZone", cfg.fishZone},
        {"RollCapDo", cfg.RollCapDo},
        {"IDLocCa", cfg.IDLocCa},
        {"locBong", cfg.locBong},
        {"magicWater", cfg.magicWater}
    };
}

void from_json(const nlohmann::json& j, PLConfig::FishingConfig& cfg) {
    if (j.contains("isCauCa")) j["isCauCa"].get_to(cfg.isCauCa);
    if (j.contains("isFakeVR")) j["isFakeVR"].get_to(cfg.isFakeVR);
    if (j.contains("isLocID")) j["isLocID"].get_to(cfg.isLocID);
    if (j.contains("isAntiLocID")) j["isAntiLocID"].get_to(cfg.isAntiLocID);
    if (j.contains("locFish")) j["locFish"].get_to(cfg.locFish);
    if (j.contains("isFishZone")) j["isFishZone"].get_to(cfg.isFishZone);
    if (j.contains("fishZone")) j["fishZone"].get_to(cfg.fishZone);
    if (j.contains("RollCapDo")) j["RollCapDo"].get_to(cfg.RollCapDo);
    if (j.contains("IDLocCa")) j["IDLocCa"].get_to(cfg.IDLocCa);
    if (j.contains("locBong")) j["locBong"].get_to(cfg.locBong);
    if (j.contains("magicWater")) j["magicWater"].get_to(cfg.magicWater);
}

void to_json(nlohmann::json& j, const PLConfig::CollectConfig::ESP& esp) {
    j = nlohmann::json{
        {"isEnable", esp.isEnable},
        {"isShowName", esp.isShowName},
        {"isTeleportButton", esp.isTeleportButton},
        {"isVein", esp.isVein},
        {"isPlants", esp.isPlants},
        {"isFossil", esp.isFossil},
        {"isSlime", esp.isSlime},
        {"isSnowman", esp.isSnowman},
        {"isOre", esp.isOre},
        {"isIng", esp.isIng},
        {"isFishingZone", esp.isFishingZone},
        {"isGathering", esp.isGathering},
        {"isCardCollect", esp.isCardCollect},
        {"isCoin", esp.isCoin},
        {"isNameTag", esp.isNameTag},
        {"isFishBreadShop", esp.isFishBreadShop},
        {"isDragonVillageMonster", esp.isDragonVillageMonster}
    };
}

void from_json(const nlohmann::json& j, PLConfig::CollectConfig::ESP& esp) {
    if (j.contains("isEnable")) j["isEnable"].get_to(esp.isEnable);
    if (j.contains("isShowName")) j["isShowName"].get_to(esp.isShowName);
    if (j.contains("isTeleportButton")) j["isTeleportButton"].get_to(esp.isTeleportButton);
    if (j.contains("isVein")) j["isVein"].get_to(esp.isVein);
    if (j.contains("isPlants")) j["isPlants"].get_to(esp.isPlants);
    if (j.contains("isFossil")) j["isFossil"].get_to(esp.isFossil);
    if (j.contains("isSlime")) j["isSlime"].get_to(esp.isSlime);
    if (j.contains("isSnowman")) j["isSnowman"].get_to(esp.isSnowman);
    if (j.contains("isOre")) j["isOre"].get_to(esp.isOre);
    if (j.contains("isIng")) j["isIng"].get_to(esp.isIng);
    if (j.contains("isFishingZone")) j["isFishingZone"].get_to(esp.isFishingZone);
    if (j.contains("isGathering")) j["isGathering"].get_to(esp.isGathering);
    if (j.contains("isCardCollect")) j["isCardCollect"].get_to(esp.isCardCollect);
    if (j.contains("isCoin")) j["isCoin"].get_to(esp.isCoin);
    if (j.contains("isNameTag")) j["isNameTag"].get_to(esp.isNameTag);
    if (j.contains("isFishBreadShop")) j["isFishBreadShop"].get_to(esp.isFishBreadShop);
    if (j.contains("isDragonVillageMonster")) j["isDragonVillageMonster"].get_to(esp.isDragonVillageMonster);
}

void to_json(nlohmann::json& j, const PLConfig::CollectConfig& cfg) {
    j = nlohmann::json{
        {"isAutoDapDa", cfg.isAutoDapDa},
        {"isAutoNguyenLieu", cfg.isAutoNguyenLieu},
        {"isAutoNhatThe", cfg.isAutoNhatThe},
        {"isTeleMapCollect", cfg.isTeleMapCollect},
        {"delayNextMap", cfg.delayNextMap},
        {"delayDapDa", cfg.delayDapDa},
        {"DSMapDa", cfg.DSMapDa},
        {"DSTypeDa", cfg.DSTypeDa},
        {"esp", cfg.esp}
    };
}

void from_json(const nlohmann::json& j, PLConfig::CollectConfig& cfg) {
    if (j.contains("isAutoDapDa")) j["isAutoDapDa"].get_to(cfg.isAutoDapDa);
    if (j.contains("isAutoNguyenLieu")) j["isAutoNguyenLieu"].get_to(cfg.isAutoNguyenLieu);
    if (j.contains("isAutoNhatThe")) j["isAutoNhatThe"].get_to(cfg.isAutoNhatThe);
    if (j.contains("isTeleMapCollect")) j["isTeleMapCollect"].get_to(cfg.isTeleMapCollect);
    if (j.contains("delayNextMap")) j["delayNextMap"].get_to(cfg.delayNextMap);
    if (j.contains("delayDapDa")) j["delayDapDa"].get_to(cfg.delayDapDa);
    if (j.contains("DSMapDa")) j["DSMapDa"].get_to(cfg.DSMapDa);
    if (j.contains("DSTypeDa")) j["DSTypeDa"].get_to(cfg.DSTypeDa);
    if (j.contains("esp")) j["esp"].get_to(cfg.esp);
}

void to_json(nlohmann::json& j, const PLConfig::MonsterConfig::ESP& esp) {
    j = nlohmann::json{
        {"isEnable", esp.isEnable},
        {"isShowName", esp.isShowName},
        {"isTeleportButton", esp.isTeleportButton}
    };
}

void from_json(const nlohmann::json& j, PLConfig::MonsterConfig::ESP& esp) {
    if (j.contains("isEnable")) j["isEnable"].get_to(esp.isEnable);
    if (j.contains("isShowName")) j["isShowName"].get_to(esp.isShowName);
    if (j.contains("isTeleportButton")) j["isTeleportButton"].get_to(esp.isTeleportButton);
}

void to_json(nlohmann::json& j, const PLConfig::MonsterConfig& cfg) {
    j = nlohmann::json{
        {"isEnable", cfg.isEnable},
        {"isAutoMonster", cfg.isAutoMonster},
        {"isCollectReward", cfg.isCollectReward},
        {"isTeleMapMonster", cfg.isTeleMapMonster},
        {"delayNextMap", cfg.delayNextMap},
        {"tocDoBanQuaiVat", cfg.tocDoBanQuaiVat},
        {"banQuaiVatHpDuoi", cfg.banQuaiVatHpDuoi},
        {"DSMapMonster", cfg.DSMapMonster},
        {"esp", cfg.esp}
    };
}

void from_json(const nlohmann::json& j, PLConfig::MonsterConfig& cfg) {
    if (j.contains("isEnable")) j["isEnable"].get_to(cfg.isEnable);
    if (j.contains("isAutoMonster")) j["isAutoMonster"].get_to(cfg.isAutoMonster);
    if (j.contains("isCollectReward")) j["isCollectReward"].get_to(cfg.isCollectReward);
    if (j.contains("isTeleMapMonster")) j["isTeleMapMonster"].get_to(cfg.isTeleMapMonster);
    if (j.contains("delayNextMap")) j["delayNextMap"].get_to(cfg.delayNextMap);
    if (j.contains("tocDoBanQuaiVat")) j["tocDoBanQuaiVat"].get_to(cfg.tocDoBanQuaiVat);
    if (j.contains("banQuaiVatHpDuoi")) j["banQuaiVatHpDuoi"].get_to(cfg.banQuaiVatHpDuoi);
    if (j.contains("DSMapMonster")) j["DSMapMonster"].get_to(cfg.DSMapMonster);
    if (j.contains("esp")) j["esp"].get_to(cfg.esp);
}

void to_json(nlohmann::json& j, const PLConfig::FarmConfig::ESP& esp) {
    j["isEnable"] = esp.isEnable;
    j["isShowName"] = esp.isShowName;
    j["isShowType"] = esp.isShowType;
    j["isTeleportButton"] = esp.isTeleportButton;
}

void from_json(const nlohmann::json& j, PLConfig::FarmConfig::ESP& esp) {
    if (j.contains("isEnable")) j["isEnable"].get_to(esp.isEnable);
    if (j.contains("isShowName")) j["isShowName"].get_to(esp.isShowName);
    if (j.contains("isShowType")) j["isShowType"].get_to(esp.isShowType);
    if (j.contains("isTeleportButton")) j["isTeleportButton"].get_to(esp.isTeleportButton);
}

void to_json(nlohmann::json& j, const PLConfig::FarmConfig& cfg) {
    j["isAutoClickCollect"] = cfg.isAutoClickCollect;
    j["isAutoPlant"] = cfg.isAutoPlant;
    j["selectedSeedId"] = cfg.selectedSeedId;
    j["selectedPlotPositions"] = cfg.selectedPlotPositions;
    j["delayPlant"] = cfg.delayPlant;
    j["isAutoReap"] = cfg.isAutoReap;
    j["selectedCropTypes"] = cfg.selectedCropTypes;
    j["selectedReapPositions"] = cfg.selectedReapPositions;
    j["delayReap"] = cfg.delayReap;
    j["esp"] = cfg.esp;
}

void from_json(const nlohmann::json& j, PLConfig::FarmConfig& cfg) {
    if (j.contains("isAutoClickCollect")) j["isAutoClickCollect"].get_to(cfg.isAutoClickCollect);
    if (j.contains("isAutoPlant")) j["isAutoPlant"].get_to(cfg.isAutoPlant);
    if (j.contains("selectedSeedId")) j["selectedSeedId"].get_to(cfg.selectedSeedId);
    if (j.contains("selectedPlotPositions")) j["selectedPlotPositions"].get_to(cfg.selectedPlotPositions);
    if (j.contains("delayPlant")) j["delayPlant"].get_to(cfg.delayPlant);
    if (j.contains("isAutoReap")) j["isAutoReap"].get_to(cfg.isAutoReap);
    if (j.contains("selectedCropTypes")) j["selectedCropTypes"].get_to(cfg.selectedCropTypes);
    if (j.contains("selectedReapPositions")) j["selectedReapPositions"].get_to(cfg.selectedReapPositions);
    if (j.contains("delayReap")) j["delayReap"].get_to(cfg.delayReap);
    if (j.contains("esp")) j["esp"].get_to(cfg.esp);
}

void to_json(nlohmann::json& j, const PLConfig::InsectConfig::ESP& esp) {
    j = nlohmann::json{
        {"isEnable", esp.isEnable},
        {"isShowName", esp.isShowName},
        {"isTeleportButton", esp.isTeleportButton},
        {"isShowGrade", esp.isShowGrade}
    };
}

void from_json(const nlohmann::json& j, PLConfig::InsectConfig::ESP& esp) {
    if (j.contains("isEnable")) j["isEnable"].get_to(esp.isEnable);
    if (j.contains("isShowName")) j["isShowName"].get_to(esp.isShowName);
    if (j.contains("isTeleportButton")) j["isTeleportButton"].get_to(esp.isTeleportButton);
    if (j.contains("isShowGrade")) j["isShowGrade"].get_to(esp.isShowGrade);
}

void to_json(nlohmann::json& j, const PLConfig::InsectConfig& cfg) {
    j = nlohmann::json{
        {"isAutoBatBo", cfg.isAutoBatBo},
        {"isFreezeCT", cfg.isFreezeCT},
        {"isTeleMapBo", cfg.isTeleMapBo},
        {"isBanBo", cfg.isBanBo},
        {"isBatBoTrenTroi", cfg.isBatBoTrenTroi},
        {"isNhatThe", cfg.isNhatThe},
        {"isDuTimTroLen", cfg.isDuTimTroLen},
        {"minInsectGrade", cfg.minInsectGrade},
        {"delayBatCT", cfg.delayBatCT},
        {"delayTeleMap", cfg.delayTeleMap},
        {"conTrungCachMatDat", cfg.conTrungCachMatDat},
        {"FullInsect", cfg.FullInsect},
        {"MaxInsectSell", cfg.MaxInsectSell},
        {"DSNenBo", cfg.DSNenBo},
        {"DSMapBo", cfg.DSMapBo},
        {"esp", cfg.esp}
    };
}

void from_json(const nlohmann::json& j, PLConfig::InsectConfig& cfg) {
    if (j.contains("isAutoBatBo")) j["isAutoBatBo"].get_to(cfg.isAutoBatBo);
    if (j.contains("isFreezeCT")) j["isFreezeCT"].get_to(cfg.isFreezeCT);
    if (j.contains("isTeleMapBo")) j["isTeleMapBo"].get_to(cfg.isTeleMapBo);
    if (j.contains("isBanBo")) j["isBanBo"].get_to(cfg.isBanBo);
    if (j.contains("isBatBoTrenTroi")) j["isBatBoTrenTroi"].get_to(cfg.isBatBoTrenTroi);
    if (j.contains("isNhatThe")) j["isNhatThe"].get_to(cfg.isNhatThe);
    if (j.contains("isDuTimTroLen")) j["isDuTimTroLen"].get_to(cfg.isDuTimTroLen);
    if (j.contains("minInsectGrade")) j["minInsectGrade"].get_to(cfg.minInsectGrade);
    if (j.contains("isXanhTroLen")) {
        bool legacyBlue = false;
        j["isXanhTroLen"].get_to(legacyBlue);
        cfg.minInsectGrade = legacyBlue ? 1 : -1;
    }
    if (j.contains("delayBatCT")) j["delayBatCT"].get_to(cfg.delayBatCT);
    if (j.contains("delayTeleMap")) j["delayTeleMap"].get_to(cfg.delayTeleMap);
    if (j.contains("conTrungCachMatDat")) j["conTrungCachMatDat"].get_to(cfg.conTrungCachMatDat);
    if (j.contains("FullInsect")) j["FullInsect"].get_to(cfg.FullInsect);
    if (j.contains("MaxInsectSell")) j["MaxInsectSell"].get_to(cfg.MaxInsectSell);
    if (j.contains("DSNenBo")) j["DSNenBo"].get_to(cfg.DSNenBo);
    if (j.contains("DSMapBo")) j["DSMapBo"].get_to(cfg.DSMapBo);
    if (j.contains("esp")) j["esp"].get_to(cfg.esp);
}

void to_json(nlohmann::json& j, const PLConfig::MiniGameConfig::DiggingConfig::EspConfig& esp) {
    j = nlohmann::json{
        {"isEnable", esp.isEnable},
        {"isShowName", esp.isShowName},
        {"isTeleportButton", esp.isTeleportButton}
    };
}

void from_json(const nlohmann::json& j, PLConfig::MiniGameConfig::DiggingConfig::EspConfig& esp) {
    if (j.contains("isEnable")) j["isEnable"].get_to(esp.isEnable);
    if (j.contains("isShowName")) j["isShowName"].get_to(esp.isShowName);
    if (j.contains("isTeleportButton")) j["isTeleportButton"].get_to(esp.isTeleportButton);
}

void to_json(nlohmann::json& j, const PLConfig::MiniGameConfig::DiggingConfig& cfg) {
    j = nlohmann::json{
        {"isEnable", cfg.isEnable},
        {"isAutoKB", cfg.isAutoKB},
        {"isLocDaoKB", cfg.isLocDaoKB},
        {"isAutoBuyXeng", cfg.isAutoBuyXeng},
        {"speedDaoKB", cfg.speedDaoKB},
        {"isAutoDigTreasure", cfg.isAutoDigTreasure},
        {"capDoAnToan", cfg.capDoAnToan},
        {"gocLechToiDa", cfg.gocLechToiDa},
        {"chuKyChuS", cfg.chuKyChuS},
        {"doCongChuS", cfg.doCongChuS},
        {"khoangCachBatDauCham", cfg.khoangCachBatDauCham},
        {"autoMoveKhiHetRuong", cfg.autoMoveKhiHetRuong},
        {"radiusTim", cfg.radiusTim},
        {"maxKhoangCach", cfg.maxKhoangCach},
        {"filterLoaiRuong", cfg.filterLoaiRuong},
        {"esp", cfg.esp}
    };
}

void from_json(const nlohmann::json& j, PLConfig::MiniGameConfig::DiggingConfig& cfg) {
    if (j.contains("isEnable")) j["isEnable"].get_to(cfg.isEnable);
    if (j.contains("isAutoKB")) j["isAutoKB"].get_to(cfg.isAutoKB);
    if (j.contains("isLocDaoKB")) j["isLocDaoKB"].get_to(cfg.isLocDaoKB);
    if (j.contains("isAutoBuyXeng")) j["isAutoBuyXeng"].get_to(cfg.isAutoBuyXeng);
    if (j.contains("speedDaoKB")) j["speedDaoKB"].get_to(cfg.speedDaoKB);
    if (j.contains("isAutoDigTreasure")) j["isAutoDigTreasure"].get_to(cfg.isAutoDigTreasure);
    if (j.contains("capDoAnToan")) j["capDoAnToan"].get_to(cfg.capDoAnToan);
    if (j.contains("gocLechToiDa")) j["gocLechToiDa"].get_to(cfg.gocLechToiDa);
    if (j.contains("chuKyChuS")) j["chuKyChuS"].get_to(cfg.chuKyChuS);
    if (j.contains("doCongChuS")) j["doCongChuS"].get_to(cfg.doCongChuS);
    if (j.contains("khoangCachBatDauCham")) j["khoangCachBatDauCham"].get_to(cfg.khoangCachBatDauCham);
    if (j.contains("autoMoveKhiHetRuong")) j["autoMoveKhiHetRuong"].get_to(cfg.autoMoveKhiHetRuong);
    if (j.contains("radiusTim")) j["radiusTim"].get_to(cfg.radiusTim);
    if (j.contains("maxKhoangCach")) j["maxKhoangCach"].get_to(cfg.maxKhoangCach);
    if (j.contains("filterLoaiRuong")) j["filterLoaiRuong"].get_to(cfg.filterLoaiRuong);
    if (j.contains("esp")) j["esp"].get_to(cfg.esp);
}

void to_json(nlohmann::json& j, const PLConfig::MiniGameConfig::Zombie& cfg) {
    j = nlohmann::json{
        {"isEnable", cfg.isEnable},
        {"isChemXa", cfg.isChemXa},
        {"isEsp", cfg.isEsp}
    };
}

void from_json(const nlohmann::json& j, PLConfig::MiniGameConfig::Zombie& cfg) {
    if (j.contains("isEnable")) j["isEnable"].get_to(cfg.isEnable);
    if (j.contains("isChemXa")) j["isChemXa"].get_to(cfg.isChemXa);
    if (j.contains("isEsp")) j["isEsp"].get_to(cfg.isEsp);
}

void to_json(nlohmann::json& j, const PLConfig::MiniGameConfig::TowerClimb& cfg) {
    j = nlohmann::json{
        {"isEnable", cfg.isEnable},
        {"delayNextPoint", cfg.delayNextPoint}
    };
}

void from_json(const nlohmann::json& j, PLConfig::MiniGameConfig::TowerClimb& cfg) {
    if (j.contains("isEnable")) j["isEnable"].get_to(cfg.isEnable);
    if (j.contains("delayNextPoint")) j["delayNextPoint"].get_to(cfg.delayNextPoint);
}

void to_json(nlohmann::json& j, const PLConfig::MiniGameConfig::Obby& cfg) {
    j = nlohmann::json{
        {"isEnable", cfg.isEnable},
        {"delayNextPoint", cfg.delayNextPoint}
    };
}

void from_json(const nlohmann::json& j, PLConfig::MiniGameConfig::Obby& cfg) {
    if (j.contains("isEnable")) j["isEnable"].get_to(cfg.isEnable);
    if (j.contains("delayNextPoint")) j["delayNextPoint"].get_to(cfg.delayNextPoint);
}

void to_json(nlohmann::json& j, const decltype(PLConfig::MiniGameConfig::ThapGa)& cfg) {
    j["isEnable"] = cfg.isEnable;
    j["delayNextPoint"] = cfg.delayNextPoint;
}

void from_json(const nlohmann::json& j, decltype(PLConfig::MiniGameConfig::ThapGa)& cfg) {
    if (j.contains("isEnable")) j["isEnable"].get_to(cfg.isEnable);
    if (j.contains("delayNextPoint")) j["delayNextPoint"].get_to(cfg.delayNextPoint);
}

void to_json(nlohmann::json& j, const decltype(PLConfig::MiniGameConfig::Party)& cfg) {
    j["isEnable"] = cfg.isEnable;
    j["delayNextPoint"] = cfg.delayNextPoint;
}

void from_json(const nlohmann::json& j, decltype(PLConfig::MiniGameConfig::Party)& cfg) {
    if (j.contains("isEnable")) j["isEnable"].get_to(cfg.isEnable);
    if (j.contains("delayNextPoint")) j["delayNextPoint"].get_to(cfg.delayNextPoint);
}

void to_json(nlohmann::json& j, const PLConfig::MiniGameConfig& cfg) {
    j["digging"] = cfg.digging;
    j["zombie"] = cfg.zombie;
    j["towerClimb"] = cfg.towerClimb;
    j["obby"] = cfg.obby;
    j["ThapGa"] = cfg.ThapGa;
    j["Party"] = cfg.Party;
}

void from_json(const nlohmann::json& j, PLConfig::MiniGameConfig& cfg) {
    if (j.contains("digging")) j["digging"].get_to(cfg.digging);
    if (j.contains("zombie")) j["zombie"].get_to(cfg.zombie);
    if (j.contains("towerClimb")) j["towerClimb"].get_to(cfg.towerClimb);
    if (j.contains("obby")) j["obby"].get_to(cfg.obby);
    if (j.contains("ThapGa")) j["ThapGa"].get_to(cfg.ThapGa);
    if (j.contains("Party")) j["Party"].get_to(cfg.Party);
}

void to_json(nlohmann::json& j, const PLConfig& cfg) {
    j = nlohmann::json{
        {"viTriCoSan", cfg.viTriCoSan},
        {"general", cfg.general},
        {"fishing", cfg.fishing},
        {"collect", cfg.collect},
        {"monster", cfg.monster},
        {"farm", cfg.farm},
        {"insect", cfg.insect},
        {"autoCatch", cfg.autoCatch},
        {"miniGame", cfg.miniGame}
    };
}

void from_json(const nlohmann::json& j, PLConfig& cfg) {
    if (j.contains("viTriCoSan")) j["viTriCoSan"].get_to(cfg.viTriCoSan);
    if (j.contains("general")) j["general"].get_to(cfg.general);
    if (j.contains("fishing")) j["fishing"].get_to(cfg.fishing);
    if (j.contains("collect")) j["collect"].get_to(cfg.collect);
    if (j.contains("monster")) j["monster"].get_to(cfg.monster);
    if (j.contains("farm")) j["farm"].get_to(cfg.farm);
    if (j.contains("insect")) j["insect"].get_to(cfg.insect);
    if (j.contains("autoCatch")) j["autoCatch"].get_to(cfg.autoCatch);
    if (j.contains("miniGame")) j["miniGame"].get_to(cfg.miniGame);
}

static std::string g_config_file_path;

static std::string GetConfigFilePath() {
    try {
        if (!g_config_file_path.empty()) return g_config_file_path;
        std::string packageName = Tools::GetPackageName();
        if (packageName.empty()) packageName = OBF("com.vng.playtogether");
        g_config_file_path = OBF("/storage/emulated/0/Android/data/") + packageName + OBF("/config.json");
        LOGD(OBF("GetConfigFilePath: %s"), g_config_file_path.c_str());
        return g_config_file_path;
    } catch (const std::exception &e) {
        LOGE(OBF("GetConfigFilePath: Exception: %s"), e.what());
        return "";
    } catch (...) {
        LOGE(OBF("GetConfigFilePath: Unknown exception"));
        return "";
    }
}

void PLConfig::Load(const std::string &content) {
    try {
        auto j = nlohmann::json::parse(content);
        from_json(j, GetConfig());
        LOGI(OBF("PLConfig::Load: Success"));
    } catch (const std::exception &e) {
        LOGE(OBF("PLConfig::Load: Exception: %s"), e.what());
    } catch (...) {
        LOGE(OBF("PLConfig::Load: Unknown exception"));
    }
}

std::string PLConfig::GetConfigContent() {
    try {
        nlohmann::json j;
        to_json(j, GetConfig());
        return j.dump(4);
    } catch (const std::exception &e) {
        return "{}";
    }
}

bool SaveConfig() {
    try {
        PLConfig &config = GetConfig();
        std::string content = config.GetConfigContent();
        if (content.empty() || content == "{}") {
            LOGE(OBF("SaveConfig: Config content is empty"));
            return false;
        }
        std::string filePath = GetConfigFilePath();
        if (filePath.empty()) {
            LOGE(OBF("SaveConfig: Config file path is empty"));
            return false;
        }
        LOGI(OBF("SaveConfig: Saving to %s"), filePath.c_str());
        fs::Result wr = fs::WriteBytes(filePath, content.data(), content.size());
        if (wr.ok()) {
            LOGI(OBF("SaveConfig: Success"));
            return true;
        }
        LOGE(OBF("SaveConfig: Failed to write file: %s"), wr.error.c_str());
        return false;
    } catch (const std::exception &e) {
        LOGE(OBF("SaveConfig: Exception: %s"), e.what());
        return false;
    }
}

bool LoadConfig() {
    try {
        std::string filePath = GetConfigFilePath();
        if (filePath.empty()) {
            LOGE(OBF("LoadConfig: Config file path is empty"));
            return false;
        }
        if (!fs::Exists(filePath)) {
            LOGI(OBF("LoadConfig: Config file does not exist: %s"), filePath.c_str());
            return false;
        }
        fs::Result rr;
        std::vector<uint8_t> bytes = fs::ReadBytes(filePath, &rr);
        if (!rr.ok() || bytes.empty()) {
            LOGE(OBF("LoadConfig: Config file is empty or unreadable"));
            return false;
        }
        std::string content(bytes.begin(), bytes.end());
        LOGI(OBF("LoadConfig: Loading from %s"), filePath.c_str());
        GetConfig().Load(content);
        LOGI(OBF("LoadConfig: Success"));
        return true;
    } catch (const std::exception &e) {
        LOGE(OBF("LoadConfig: Exception: %s"), e.what());
        return false;
    } catch (...) {
        LOGE(OBF("LoadConfig: Unknown exception"));
        return false;
    }
}

