#include "PLConfig.h"
#include "Config.h"
#include <json/json.hpp>
#include <FileManager.hpp>
#include <Tools/Tools.h>
#include <Includes/obfuscate.h>
#define LOG_TAG OBF("ATTACK_PlayTogether")
#include <Includes/Logger.h>

void to_json(nlohmann::json& j, const std::pair<unsigned int, unsigned int>& p) {
    j = nlohmann::json{{"zoneId", p.first}, {"baitItemId", p.second}};
}

void from_json(const nlohmann::json& j, std::pair<unsigned int, unsigned int>& p) {
    if (j.contains("zoneId")) j["zoneId"].get_to(p.first);
    if (j.contains("baitItemId")) j["baitItemId"].get_to(p.second);
}

void to_json(nlohmann::json& j, const std::pair<unsigned int, std::vector<unsigned int>>& p) {
    j = nlohmann::json{{"levelId", p.first}, {"fishIds", p.second}};
}

void from_json(const nlohmann::json& j, std::pair<unsigned int, std::vector<unsigned int>>& p) {
    if (j.contains("levelId")) j["levelId"].get_to(p.first);
    if (j.contains("fishIds")) j["fishIds"].get_to(p.second);
}

void to_json(nlohmann::json& j, const PLConfig::GeneralConfig& cfg) {
    j = nlohmann::json{{"isInfo", cfg.isInfo}};
}

void from_json(const nlohmann::json& j, PLConfig::GeneralConfig& cfg) {
    if (j.contains("isInfo")) j["isInfo"].get_to(cfg.isInfo);
}

void to_json(nlohmann::json& j, const PLConfig::FishingConfig& cfg) {
    j = nlohmann::json{
        {"enabled", cfg.enabled},
        {"autoCloseReward", cfg.autoCloseReward},
        {"autoSellTrash", cfg.autoSellTrash},
        {"maxSellGrade", cfg.maxSellGrade},
        {"stopWhenCountOver", cfg.stopWhenCountOver},
        {"smartKeepSell", cfg.smartKeepSell},
        {"smartKeepMinGrade", cfg.smartKeepMinGrade},
        {"smartKeepMaxOwned", cfg.smartKeepMaxOwned},
        {"autoEquipBait", cfg.autoEquipBait},
        {"baitItemId", cfg.baitItemId},
        {"smartBaitByZone", cfg.smartBaitByZone},
        {"smartBaitAutoEffect", cfg.smartBaitAutoEffect},
        {"baitZonePrefs", cfg.baitZonePrefs},
        {"minSellValue", cfg.minSellValue},
        {"keepCodexFish", cfg.keepCodexFish},
        {"filterByShadow", cfg.filterByShadow},
        {"keepShadow", std::vector<bool>(cfg.keepShadow, cfg.keepShadow + 7)},
        {"filterByLevel", cfg.filterByLevel},
        {"levelMin", cfg.levelMin},
        {"levelMax", cfg.levelMax},
        {"keepLevelIds", cfg.keepLevelIds},
        {"learnedLevelFish", cfg.learnedLevelFish}
    };
}

void from_json(const nlohmann::json& j, PLConfig::FishingConfig& cfg) {
    if (j.contains("enabled")) j["enabled"].get_to(cfg.enabled);
    if (j.contains("autoCloseReward")) j["autoCloseReward"].get_to(cfg.autoCloseReward);
    if (j.contains("autoSellTrash")) j["autoSellTrash"].get_to(cfg.autoSellTrash);
    if (j.contains("maxSellGrade")) j["maxSellGrade"].get_to(cfg.maxSellGrade);
    if (j.contains("stopWhenCountOver")) j["stopWhenCountOver"].get_to(cfg.stopWhenCountOver);
    if (j.contains("smartKeepSell")) j["smartKeepSell"].get_to(cfg.smartKeepSell);
    if (j.contains("smartKeepMinGrade")) j["smartKeepMinGrade"].get_to(cfg.smartKeepMinGrade);
    if (j.contains("smartKeepMaxOwned")) j["smartKeepMaxOwned"].get_to(cfg.smartKeepMaxOwned);
    if (j.contains("autoEquipBait")) j["autoEquipBait"].get_to(cfg.autoEquipBait);
    if (j.contains("baitItemId")) j["baitItemId"].get_to(cfg.baitItemId);
    if (j.contains("smartBaitByZone")) j["smartBaitByZone"].get_to(cfg.smartBaitByZone);
    if (j.contains("smartBaitAutoEffect")) j["smartBaitAutoEffect"].get_to(cfg.smartBaitAutoEffect);
    if (j.contains("baitZonePrefs")) j["baitZonePrefs"].get_to(cfg.baitZonePrefs);
    if (j.contains("minSellValue")) j["minSellValue"].get_to(cfg.minSellValue);
    if (j.contains("keepCodexFish")) j["keepCodexFish"].get_to(cfg.keepCodexFish);
    if (j.contains("filterByShadow")) j["filterByShadow"].get_to(cfg.filterByShadow);
    if (j.contains("keepShadow") && j["keepShadow"].is_array()) {
        auto arr = j["keepShadow"];
        for (size_t i = 0; i < arr.size() && i < 7; i++) cfg.keepShadow[i] = arr[i].get<bool>();
    }
    if (j.contains("filterByLevel")) j["filterByLevel"].get_to(cfg.filterByLevel);
    if (j.contains("levelMin")) j["levelMin"].get_to(cfg.levelMin);
    if (j.contains("levelMax")) j["levelMax"].get_to(cfg.levelMax);
    if (j.contains("keepLevelIds")) j["keepLevelIds"].get_to(cfg.keepLevelIds);
    if (j.contains("learnedLevelFish")) j["learnedLevelFish"].get_to(cfg.learnedLevelFish);
}

void to_json(nlohmann::json& j, const PLConfig& cfg) {
    j = nlohmann::json{{"general", cfg.general}, {"fishing", cfg.fishing}};
}

void from_json(const nlohmann::json& j, PLConfig& cfg) {
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
