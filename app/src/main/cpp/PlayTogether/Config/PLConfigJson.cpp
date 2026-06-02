#include "PLConfig.h"
#include "Config.h"
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
        {"isRepair", cfg.isRepair},
        {"isBoQuaLoiThoaiNPC", cfg.isBoQuaLoiThoaiNPC},
        {"isResetTrangThai", cfg.isResetTrangThai},
        {"chaynhanh", cfg.chaynhanh},
        {"nhaycao", cfg.nhaycao}
    };
}

void from_json(const nlohmann::json& j, PLConfig::GeneralConfig& cfg) {
    if (j.contains("isInfo")) j["isInfo"].get_to(cfg.isInfo);
    if (j.contains("isRepair")) j["isRepair"].get_to(cfg.isRepair);
    if (j.contains("isBoQuaLoiThoaiNPC")) j["isBoQuaLoiThoaiNPC"].get_to(cfg.isBoQuaLoiThoaiNPC);
    if (j.contains("isResetTrangThai")) j["isResetTrangThai"].get_to(cfg.isResetTrangThai);
    if (j.contains("chaynhanh")) j["chaynhanh"].get_to(cfg.chaynhanh);
    if (j.contains("nhaycao")) j["nhaycao"].get_to(cfg.nhaycao);
}

void to_json(nlohmann::json& j, const PLConfig::FishingConfig::MagicWater& mw) {
    j = nlohmann::json{
        {"isEnable", mw.isEnable},
        {"levelUses", {mw.levelUses[0], mw.levelUses[1], mw.levelUses[2]}}
    };
}

void from_json(const nlohmann::json& j, PLConfig::FishingConfig::MagicWater& mw) {
    if (j.contains("isEnable")) j["isEnable"].get_to(mw.isEnable);
    if (j.contains("levelUses")) {
        auto arr = j["levelUses"];
        for (int i = 0; i < 3; ++i) {
            if (i < (int) arr.size()) arr[i].get_to(mw.levelUses[i]);
            else mw.levelUses[i] = 0;
        }
    } else {
        int oldLevel = 0;
        int oldMaxUses = 0;
        if (j.contains("level")) j["level"].get_to(oldLevel);
        if (j.contains("maxUses")) j["maxUses"].get_to(oldMaxUses);
        mw.levelUses[0] = mw.levelUses[1] = mw.levelUses[2] = 0;
        if (oldLevel >= 0 && oldLevel < 3 && oldMaxUses > 0) mw.levelUses[oldLevel] = oldMaxUses;
    }
}

void to_json(nlohmann::json& j, const PLConfig::FishingConfig::ESP& esp) {
    j = nlohmann::json{
        {"isEnable", esp.isEnable},
        {"isShowZone", esp.isShowZone},
        {"isShowStatus", esp.isShowStatus}
    };
}

void from_json(const nlohmann::json& j, PLConfig::FishingConfig::ESP& esp) {
    if (j.contains("isEnable")) j["isEnable"].get_to(esp.isEnable);
    if (j.contains("isShowZone")) j["isShowZone"].get_to(esp.isShowZone);
    if (j.contains("isShowStatus")) j["isShowStatus"].get_to(esp.isShowStatus);
}

void to_json(nlohmann::json& j, const PLConfig::FishingConfig& cfg) {
    j = nlohmann::json{
        {"isCauCa", cfg.isCauCa},
        {"isFakeVR", cfg.isFakeVR},
        {"isLocID", cfg.isLocID},
        {"locFish", cfg.locFish},
        {"isFishZone", cfg.isFishZone},
        {"fishZone", cfg.fishZone},
        {"RollCapDo", cfg.RollCapDo},
        {"delayAutoMs", cfg.delayAutoMs},
        {"IDLocCa", cfg.IDLocCa},
        {"locBong", cfg.locBong},
        {"isBaoQuan", cfg.isBaoQuan},
        {"isBanGoi", cfg.isBanGoi},
        {"isDuNenVip", cfg.isDuNenVip},
        {"isDuB67", cfg.isDuB67},
        {"magicWater", cfg.magicWater},
        {"esp", cfg.esp}
    };
}

void from_json(const nlohmann::json& j, PLConfig::FishingConfig& cfg) {
    if (j.contains("isCauCa")) j["isCauCa"].get_to(cfg.isCauCa);
    if (j.contains("isFakeVR")) j["isFakeVR"].get_to(cfg.isFakeVR);
    if (j.contains("isLocID")) j["isLocID"].get_to(cfg.isLocID);
    if (j.contains("locFish")) j["locFish"].get_to(cfg.locFish);
    if (j.contains("isFishZone")) j["isFishZone"].get_to(cfg.isFishZone);
    if (j.contains("fishZone")) j["fishZone"].get_to(cfg.fishZone);
    if (j.contains("RollCapDo")) j["RollCapDo"].get_to(cfg.RollCapDo);
    if (j.contains("delayAutoMs")) j["delayAutoMs"].get_to(cfg.delayAutoMs);
    if (j.contains("IDLocCa")) j["IDLocCa"].get_to(cfg.IDLocCa);
    if (j.contains("locBong")) j["locBong"].get_to(cfg.locBong);
    if (j.contains("isBaoQuan")) j["isBaoQuan"].get_to(cfg.isBaoQuan);
    if (j.contains("isBanGoi")) j["isBanGoi"].get_to(cfg.isBanGoi);
    if (j.contains("isDuNenVip")) j["isDuNenVip"].get_to(cfg.isDuNenVip);
    if (j.contains("isDuB67")) j["isDuB67"].get_to(cfg.isDuB67);
    if (j.contains("magicWater")) j["magicWater"].get_to(cfg.magicWater);
    if (j.contains("esp")) j["esp"].get_to(cfg.esp);
    if (j.contains("general")) {
        const auto &g = j["general"];
        if (g.contains("isBaoQuan")) g["isBaoQuan"].get_to(cfg.isBaoQuan);
        if (g.contains("isBanGoi")) g["isBanGoi"].get_to(cfg.isBanGoi);
        if (g.contains("isDuNenVip")) g["isDuNenVip"].get_to(cfg.isDuNenVip);
        if (g.contains("isDuB67")) g["isDuB67"].get_to(cfg.isDuB67);
    }
}

void to_json(nlohmann::json& j, const PLConfig& cfg) {
    j = nlohmann::json{
        {"viTriCoSan", cfg.viTriCoSan},
        {"general", cfg.general},
        {"fishing", cfg.fishing}
    };
}

void from_json(const nlohmann::json& j, PLConfig& cfg) {
    if (j.contains("viTriCoSan")) j["viTriCoSan"].get_to(cfg.viTriCoSan);
    if (j.contains("general")) j["general"].get_to(cfg.general);
    if (j.contains("fishing")) j["fishing"].get_to(cfg.fishing);
}

static std::string g_config_file_path;

static std::string GetConfigFilePath() {
    try {
        if (!g_config_file_path.empty()) return g_config_file_path;
        std::string packageName = Tools::GetPackageName();
        if (packageName.empty()) packageName = OBF("com.vng.playtogether");
        g_config_file_path = OBF("/storage/emulated/0/Android/data/") + packageName + OBF("/config.json");
        return g_config_file_path;
    } catch (...) {
        return "";
    }
}

void PLConfig::Load(const std::string &content) {
    try {
        auto j = nlohmann::json::parse(content);
        from_json(j, GetConfig());
    } catch (const std::exception &e) {
        LOGE(OBF("PLConfig::Load: %s"), e.what());
    }
}

std::string PLConfig::GetConfigContent() {
    try {
        nlohmann::json j;
        to_json(j, GetConfig());
        return j.dump(4);
    } catch (...) {
        return "{}";
    }
}

bool SaveConfig() {
    try {
        std::string content = GetConfig().GetConfigContent();
        if (content.empty() || content == "{}") return false;
        std::string filePath = GetConfigFilePath();
        if (filePath.empty()) return false;
        fs::Result wr = fs::WriteBytes(filePath, content.data(), content.size());
        return wr.ok();
    } catch (...) {
        return false;
    }
}

bool LoadConfig() {
    try {
        std::string filePath = GetConfigFilePath();
        if (filePath.empty() || !fs::Exists(filePath)) return false;
        fs::Result rr;
        std::vector<uint8_t> bytes = fs::ReadBytes(filePath, &rr);
        if (!rr.ok() || bytes.empty()) return false;
        std::string content(bytes.begin(), bytes.end());
        GetConfig().Load(content);
        return true;
    } catch (...) {
        return false;
    }
}
