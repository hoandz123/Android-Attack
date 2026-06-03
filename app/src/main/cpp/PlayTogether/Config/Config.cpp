#include "Config.h"
#include <json/json.hpp>
#include <FileManager.hpp>
#include <Tools/Tools.h>
#include <Includes/obfuscate.h>
#define LOGGER_TAG "ATTACK_PlayTogether"
#include <Includes/Logger.h>

bool isGameLoading = false;
PLConfig gPLConfig;

void to_json(nlohmann::json &j, const PLConfig::FishingConfig &cfg) {
    j = nlohmann::json{
        {"enabled", cfg.enabled},
        {"autoCloseReward", cfg.autoCloseReward},
        {"stopWhenCountOver", cfg.stopWhenCountOver},
        {"autoEquipBait", cfg.autoEquipBait},
        {"baitItemId", cfg.baitItemId},
        {"autoCraftBait", cfg.autoCraftBait},
        {"craftBaitItemId", cfg.craftBaitItemId},
        {"craftBaitTargetCount", cfg.craftBaitTargetCount},
        {"fakeZoneEnabled", cfg.fakeZoneEnabled},
        {"fakeZoneId", cfg.fakeZoneId},
        {"filterByShadow", cfg.filterByShadow},
        {"keepShadow", std::vector<bool>(cfg.keepShadow, cfg.keepShadow + 7)},
        {"filterByLevel", cfg.filterByLevel},
        {"keepLevels", cfg.keepLevels},
        {"sellByShadow", cfg.sellByShadow},
        {"sellShadow", std::vector<bool>(cfg.sellShadow, cfg.sellShadow + 7)},
        {"sellByGrade", cfg.sellByGrade},
        {"sellGrade", std::vector<bool>(cfg.sellGrade, cfg.sellGrade + 5)}
    };
}

void from_json(const nlohmann::json &j, PLConfig::FishingConfig &cfg) {
    if (j.contains("enabled")) j["enabled"].get_to(cfg.enabled);
    if (j.contains("autoCloseReward")) j["autoCloseReward"].get_to(cfg.autoCloseReward);
    if (j.contains("stopWhenCountOver")) j["stopWhenCountOver"].get_to(cfg.stopWhenCountOver);
    if (j.contains("autoEquipBait")) j["autoEquipBait"].get_to(cfg.autoEquipBait);
    if (j.contains("baitItemId")) j["baitItemId"].get_to(cfg.baitItemId);
    if (j.contains("autoCraftBait")) j["autoCraftBait"].get_to(cfg.autoCraftBait);
    if (j.contains("craftBaitItemId")) j["craftBaitItemId"].get_to(cfg.craftBaitItemId);
    if (j.contains("craftBaitTargetCount")) j["craftBaitTargetCount"].get_to(cfg.craftBaitTargetCount);
    if (!j.contains("craftBaitTargetCount") && j.contains("craftBaitMinCount")) {
        j["craftBaitMinCount"].get_to(cfg.craftBaitTargetCount);
    }
    if (j.contains("fakeZoneEnabled")) j["fakeZoneEnabled"].get_to(cfg.fakeZoneEnabled);
    if (j.contains("fakeZoneId")) j["fakeZoneId"].get_to(cfg.fakeZoneId);
    if (j.contains("filterByShadow")) j["filterByShadow"].get_to(cfg.filterByShadow);
    if (j.contains("keepShadow") && j["keepShadow"].is_array()) {
        auto arr = j["keepShadow"];
        for (size_t i = 0; i < arr.size() && i < 7; i++) cfg.keepShadow[i] = arr[i].get<bool>();
    }
    if (j.contains("filterByLevel")) j["filterByLevel"].get_to(cfg.filterByLevel);
    if (j.contains("keepLevels")) j["keepLevels"].get_to(cfg.keepLevels);
    if (j.contains("sellByShadow")) j["sellByShadow"].get_to(cfg.sellByShadow);
    if (j.contains("sellShadow") && j["sellShadow"].is_array()) {
        auto arr = j["sellShadow"];
        for (size_t i = 0; i < arr.size() && i < 7; i++) cfg.sellShadow[i] = arr[i].get<bool>();
    }
    if (j.contains("sellByGrade")) j["sellByGrade"].get_to(cfg.sellByGrade);
    if (j.contains("sellGrade") && j["sellGrade"].is_array()) {
        auto arr = j["sellGrade"];
        for (size_t i = 0; i < arr.size() && i < 5; i++) cfg.sellGrade[i] = arr[i].get<bool>();
    }
    if (!j.contains("sellByGrade") && j.contains("autoSellTrash") && j["autoSellTrash"].get<bool>()) {
        cfg.sellByGrade = true;
        int maxG = 2;
        if (j.contains("maxSellGrade")) maxG = j["maxSellGrade"].get<int>();
        if (maxG < 1) maxG = 1;
        if (maxG > 5) maxG = 5;
        for (int i = 0; i < maxG; i++) cfg.sellGrade[i] = true;
    }
    if (cfg.keepLevels.empty() && j.contains("keepLevelIds") && j["keepLevelIds"].is_array()) {
        std::string migrated;
        for (const auto &v : j["keepLevelIds"]) {
            if (!migrated.empty()) migrated += ',';
            migrated += std::to_string(v.get<unsigned int>());
        }
        cfg.keepLevels = migrated;
    }
}

void to_json(nlohmann::json &j, const PLConfig &cfg) {
    j = nlohmann::json{{"fishing", cfg.fishing}};
}

void from_json(const nlohmann::json &j, PLConfig &cfg) {
    if (j.contains("fishing")) j["fishing"].get_to(cfg.fishing);
}

namespace {

std::string GetConfigFilePath() {
    try {
        static std::string path;
        if (!path.empty()) return path;
        std::string packageName = Tools::GetPackageName();
        if (packageName.empty()) packageName = OBF("com.vng.playtogether");
        path = OBF("/storage/emulated/0/Android/data/") + packageName + OBF("/config.json");
        return path;
    } catch (...) {
        return "";
    }
}

} // namespace

void PLConfig::Load(const std::string &content) {
    try {
        auto j = nlohmann::json::parse(content);
        from_json(j, gPLConfig);
    } catch (const std::exception &e) {
        LOGE(OBF("PLConfig::Load: %s"), e.what());
    }
}

std::string PLConfig::GetConfigContent() {
    try {
        nlohmann::json j;
        to_json(j, gPLConfig);
        return j.dump(4);
    } catch (...) {
        return "{}";
    }
}

bool SaveConfig() {
    try {
        std::string content = gPLConfig.GetConfigContent();
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
        gPLConfig.Load(content);
        return true;
    } catch (...) {
        return false;
    }
}
