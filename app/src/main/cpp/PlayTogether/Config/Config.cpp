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
        {OBF("enabled"), cfg.enabled},
        {OBF("autoCloseReward"), cfg.autoCloseReward},
        {OBF("stopWhenCountOver"), cfg.stopWhenCountOver},
        {OBF("autoEquipBait"), cfg.autoEquipBait},
        {OBF("baitItemId"), cfg.baitItemId},
        {OBF("autoCraftBait"), cfg.autoCraftBait},
        {OBF("craftBaitItemId"), cfg.craftBaitItemId},
        {OBF("craftBaitTargetCount"), cfg.craftBaitTargetCount},
        {OBF("fakeZoneEnabled"), cfg.fakeZoneEnabled},
        {OBF("fakeZoneId"), cfg.fakeZoneId},
        {OBF("filterByShadow"), cfg.filterByShadow},
        {OBF("keepShadow"), std::vector<bool>(cfg.keepShadow, cfg.keepShadow + 7)},
        {OBF("filterByLevel"), cfg.filterByLevel},
        {OBF("keepLevels"), cfg.keepLevels},
        {OBF("sellByShadow"), cfg.sellByShadow},
        {OBF("sellShadow"), std::vector<bool>(cfg.sellShadow, cfg.sellShadow + 7)},
        {OBF("sellByGrade"), cfg.sellByGrade},
        {OBF("sellGrade"), std::vector<bool>(cfg.sellGrade, cfg.sellGrade + 5)}
    };
}

void from_json(const nlohmann::json &j, PLConfig::FishingConfig &cfg) {
    if (j.contains(OBF("enabled"))) j[OBF("enabled")].get_to(cfg.enabled);
    if (j.contains(OBF("autoCloseReward"))) j[OBF("autoCloseReward")].get_to(cfg.autoCloseReward);
    if (j.contains(OBF("stopWhenCountOver"))) j[OBF("stopWhenCountOver")].get_to(cfg.stopWhenCountOver);
    if (j.contains(OBF("autoEquipBait"))) j[OBF("autoEquipBait")].get_to(cfg.autoEquipBait);
    if (j.contains(OBF("baitItemId"))) j[OBF("baitItemId")].get_to(cfg.baitItemId);
    if (j.contains(OBF("autoCraftBait"))) j[OBF("autoCraftBait")].get_to(cfg.autoCraftBait);
    if (j.contains(OBF("craftBaitItemId"))) j[OBF("craftBaitItemId")].get_to(cfg.craftBaitItemId);
    if (j.contains(OBF("craftBaitTargetCount"))) j[OBF("craftBaitTargetCount")].get_to(cfg.craftBaitTargetCount);
    if (!j.contains(OBF("craftBaitTargetCount")) && j.contains(OBF("craftBaitMinCount"))) {
        j[OBF("craftBaitMinCount")].get_to(cfg.craftBaitTargetCount);
    }
    if (j.contains(OBF("fakeZoneEnabled"))) j[OBF("fakeZoneEnabled")].get_to(cfg.fakeZoneEnabled);
    if (j.contains(OBF("fakeZoneId"))) j[OBF("fakeZoneId")].get_to(cfg.fakeZoneId);
    if (j.contains(OBF("filterByShadow"))) j[OBF("filterByShadow")].get_to(cfg.filterByShadow);
    if (j.contains(OBF("keepShadow")) && j[OBF("keepShadow")].is_array()) {
        auto arr = j[OBF("keepShadow")];
        for (size_t i = 0; i < arr.size() && i < 7; i++) cfg.keepShadow[i] = arr[i].get<bool>();
    }
    if (j.contains(OBF("filterByLevel"))) j[OBF("filterByLevel")].get_to(cfg.filterByLevel);
    if (j.contains(OBF("keepLevels"))) j[OBF("keepLevels")].get_to(cfg.keepLevels);
    if (j.contains(OBF("sellByShadow"))) j[OBF("sellByShadow")].get_to(cfg.sellByShadow);
    if (j.contains(OBF("sellShadow")) && j[OBF("sellShadow")].is_array()) {
        auto arr = j[OBF("sellShadow")];
        for (size_t i = 0; i < arr.size() && i < 7; i++) cfg.sellShadow[i] = arr[i].get<bool>();
    }
    if (j.contains(OBF("sellByGrade"))) j[OBF("sellByGrade")].get_to(cfg.sellByGrade);
    if (j.contains(OBF("sellGrade")) && j[OBF("sellGrade")].is_array()) {
        auto arr = j[OBF("sellGrade")];
        for (size_t i = 0; i < arr.size() && i < 5; i++) cfg.sellGrade[i] = arr[i].get<bool>();
    }
    if (!j.contains(OBF("sellByGrade")) && j.contains(OBF("autoSellTrash")) && j[OBF("autoSellTrash")].get<bool>()) {
        cfg.sellByGrade = true;
        int maxG = 2;
        if (j.contains(OBF("maxSellGrade"))) maxG = j[OBF("maxSellGrade")].get<int>();
        if (maxG < 1) maxG = 1;
        if (maxG > 5) maxG = 5;
        for (int i = 0; i < maxG; i++) cfg.sellGrade[i] = true;
    }
    if (cfg.keepLevels.empty() && j.contains(OBF("keepLevelIds")) && j[OBF("keepLevelIds")].is_array()) {
        std::string migrated;
        for (const auto &v : j[OBF("keepLevelIds")]) {
            if (!migrated.empty()) migrated += ',';
            migrated += std::to_string(v.get<unsigned int>());
        }
        cfg.keepLevels = migrated;
    }
}

void to_json(nlohmann::json &j, const PLConfig &cfg) {
    j = nlohmann::json{{OBF("fishing"), cfg.fishing}};
}

void from_json(const nlohmann::json &j, PLConfig &cfg) {
    if (j.contains(OBF("fishing"))) j[OBF("fishing")].get_to(cfg.fishing);
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
