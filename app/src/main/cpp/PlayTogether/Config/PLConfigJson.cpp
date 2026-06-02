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
        {"showStatus", cfg.showStatus},
        {"handleBigFish", cfg.handleBigFish},
        {"tickIntervalMs", cfg.tickIntervalMs},
        {"actionIntervalMs", cfg.actionIntervalMs},
        {"restartDelayMs", cfg.restartDelayMs},
        {"hitDelayMs", cfg.hitDelayMs},
        {"liftDelayMs", cfg.liftDelayMs},
        {"autoSellTrash", cfg.autoSellTrash},
        {"maxSellGrade", cfg.maxSellGrade},
        {"pauseOnRareCatch", cfg.pauseOnRareCatch},
        {"minRareGrade", cfg.minRareGrade},
        {"showSessionStats", cfg.showSessionStats},
        {"showFloatMarker", cfg.showFloatMarker},
        {"showZoneInfo", cfg.showZoneInfo},
        {"showFailHint", cfg.showFailHint},
        {"showEfficiency", cfg.showEfficiency},
        {"showBigFishHp", cfg.showBigFishHp},
        {"adaptiveCastBackoff", cfg.adaptiveCastBackoff},
        {"stopWhenCountOver", cfg.stopWhenCountOver},
        {"skipBoastDelay", cfg.skipBoastDelay},
        {"boastSkipMs", cfg.boastSkipMs},
        {"autoPerfectTug", cfg.autoPerfectTug},
        {"perfectLiftIntervalMs", cfg.perfectLiftIntervalMs},
        {"fastBite", cfg.fastBite},
        {"smartKeepSell", cfg.smartKeepSell},
        {"smartKeepMinGrade", cfg.smartKeepMinGrade},
        {"smartKeepMaxOwned", cfg.smartKeepMaxOwned},
        {"autoEquipBait", cfg.autoEquipBait},
        {"baitItemId", cfg.baitItemId},
        {"smartBaitByZone", cfg.smartBaitByZone},
        {"smartBaitAutoEffect", cfg.smartBaitAutoEffect},
        {"baitZonePrefs", cfg.baitZonePrefs},
        {"guideRouting", cfg.guideRouting},
        {"guidePointId", cfg.guidePointId},
        {"guideTargetZoneId", cfg.guideTargetZoneId},
        {"guideFailStreak", cfg.guideFailStreak},
        {"autoCatchNetCheck", cfg.autoCatchNetCheck},
        {"autoCatchIntervalMs", cfg.autoCatchIntervalMs},
        {"autoDailyMissionReward", cfg.autoDailyMissionReward},
        {"missionClaimIntervalMs", cfg.missionClaimIntervalMs},
        {"targetFishItemId", cfg.targetFishItemId},
        {"adaptivePacing", cfg.adaptivePacing},
        {"stunOrchestrator", cfg.stunOrchestrator},
        {"stunHitIntervalMs", cfg.stunHitIntervalMs},
        {"maxStunHitsPerPhase", cfg.maxStunHitsPerPhase},
        {"minSellValue", cfg.minSellValue},
        {"keepCodexFish", cfg.keepCodexFish},
        {"autoRaidEnter", cfg.autoRaidEnter},
        {"filterByShadow", cfg.filterByShadow},
        {"keepShadow", std::vector<bool>(cfg.keepShadow, cfg.keepShadow + 7)},
        {"filterByLevel", cfg.filterByLevel},
        {"levelMin", cfg.levelMin},
        {"levelMax", cfg.levelMax},
        {"keepLevelIds", cfg.keepLevelIds}
    };
}

void from_json(const nlohmann::json& j, PLConfig::FishingConfig& cfg) {
    if (j.contains("enabled")) j["enabled"].get_to(cfg.enabled);
    if (j.contains("autoCloseReward")) j["autoCloseReward"].get_to(cfg.autoCloseReward);
    if (j.contains("showStatus")) j["showStatus"].get_to(cfg.showStatus);
    if (j.contains("handleBigFish")) j["handleBigFish"].get_to(cfg.handleBigFish);
    if (j.contains("tickIntervalMs")) j["tickIntervalMs"].get_to(cfg.tickIntervalMs);
    if (j.contains("actionIntervalMs")) j["actionIntervalMs"].get_to(cfg.actionIntervalMs);
    if (j.contains("restartDelayMs")) j["restartDelayMs"].get_to(cfg.restartDelayMs);
    if (j.contains("hitDelayMs")) j["hitDelayMs"].get_to(cfg.hitDelayMs);
    if (j.contains("liftDelayMs")) j["liftDelayMs"].get_to(cfg.liftDelayMs);
    if (j.contains("autoSellTrash")) j["autoSellTrash"].get_to(cfg.autoSellTrash);
    if (j.contains("maxSellGrade")) j["maxSellGrade"].get_to(cfg.maxSellGrade);
    if (j.contains("pauseOnRareCatch")) j["pauseOnRareCatch"].get_to(cfg.pauseOnRareCatch);
    if (j.contains("minRareGrade")) j["minRareGrade"].get_to(cfg.minRareGrade);
    if (j.contains("showSessionStats")) j["showSessionStats"].get_to(cfg.showSessionStats);
    if (j.contains("showFloatMarker")) j["showFloatMarker"].get_to(cfg.showFloatMarker);
    if (j.contains("showZoneInfo")) j["showZoneInfo"].get_to(cfg.showZoneInfo);
    if (j.contains("showFailHint")) j["showFailHint"].get_to(cfg.showFailHint);
    if (j.contains("showEfficiency")) j["showEfficiency"].get_to(cfg.showEfficiency);
    if (j.contains("showBigFishHp")) j["showBigFishHp"].get_to(cfg.showBigFishHp);
    if (j.contains("adaptiveCastBackoff")) j["adaptiveCastBackoff"].get_to(cfg.adaptiveCastBackoff);
    if (j.contains("stopWhenCountOver")) j["stopWhenCountOver"].get_to(cfg.stopWhenCountOver);
    if (j.contains("skipBoastDelay")) j["skipBoastDelay"].get_to(cfg.skipBoastDelay);
    if (j.contains("boastSkipMs")) j["boastSkipMs"].get_to(cfg.boastSkipMs);
    if (j.contains("autoPerfectTug")) j["autoPerfectTug"].get_to(cfg.autoPerfectTug);
    if (j.contains("perfectLiftIntervalMs")) j["perfectLiftIntervalMs"].get_to(cfg.perfectLiftIntervalMs);
    if (j.contains("fastBite")) j["fastBite"].get_to(cfg.fastBite);
    if (j.contains("smartKeepSell")) j["smartKeepSell"].get_to(cfg.smartKeepSell);
    if (j.contains("smartKeepMinGrade")) j["smartKeepMinGrade"].get_to(cfg.smartKeepMinGrade);
    if (j.contains("smartKeepMaxOwned")) j["smartKeepMaxOwned"].get_to(cfg.smartKeepMaxOwned);
    if (j.contains("autoEquipBait")) j["autoEquipBait"].get_to(cfg.autoEquipBait);
    if (j.contains("baitItemId")) j["baitItemId"].get_to(cfg.baitItemId);
    if (j.contains("smartBaitByZone")) j["smartBaitByZone"].get_to(cfg.smartBaitByZone);
    if (j.contains("smartBaitAutoEffect")) j["smartBaitAutoEffect"].get_to(cfg.smartBaitAutoEffect);
    if (j.contains("baitZonePrefs")) j["baitZonePrefs"].get_to(cfg.baitZonePrefs);
    if (j.contains("guideRouting")) j["guideRouting"].get_to(cfg.guideRouting);
    if (j.contains("guidePointId")) j["guidePointId"].get_to(cfg.guidePointId);
    if (j.contains("guideTargetZoneId")) j["guideTargetZoneId"].get_to(cfg.guideTargetZoneId);
    if (j.contains("guideFailStreak")) j["guideFailStreak"].get_to(cfg.guideFailStreak);
    if (j.contains("autoCatchNetCheck")) j["autoCatchNetCheck"].get_to(cfg.autoCatchNetCheck);
    if (j.contains("autoCatchIntervalMs")) j["autoCatchIntervalMs"].get_to(cfg.autoCatchIntervalMs);
    if (j.contains("autoDailyMissionReward")) j["autoDailyMissionReward"].get_to(cfg.autoDailyMissionReward);
    if (j.contains("missionClaimIntervalMs")) j["missionClaimIntervalMs"].get_to(cfg.missionClaimIntervalMs);
    if (j.contains("targetFishItemId")) j["targetFishItemId"].get_to(cfg.targetFishItemId);
    if (j.contains("adaptivePacing")) j["adaptivePacing"].get_to(cfg.adaptivePacing);
    if (j.contains("stunOrchestrator")) j["stunOrchestrator"].get_to(cfg.stunOrchestrator);
    if (j.contains("stunHitIntervalMs")) j["stunHitIntervalMs"].get_to(cfg.stunHitIntervalMs);
    if (j.contains("maxStunHitsPerPhase")) j["maxStunHitsPerPhase"].get_to(cfg.maxStunHitsPerPhase);
    if (j.contains("minSellValue")) j["minSellValue"].get_to(cfg.minSellValue);
    if (j.contains("keepCodexFish")) j["keepCodexFish"].get_to(cfg.keepCodexFish);
    if (j.contains("autoRaidEnter")) j["autoRaidEnter"].get_to(cfg.autoRaidEnter);
    if (j.contains("filterByShadow")) j["filterByShadow"].get_to(cfg.filterByShadow);
    if (j.contains("keepShadow") && j["keepShadow"].is_array()) {
        auto arr = j["keepShadow"];
        for (size_t i = 0; i < arr.size() && i < 7; i++) cfg.keepShadow[i] = arr[i].get<bool>();
    }
    if (j.contains("filterByLevel")) j["filterByLevel"].get_to(cfg.filterByLevel);
    if (j.contains("levelMin")) j["levelMin"].get_to(cfg.levelMin);
    if (j.contains("levelMax")) j["levelMax"].get_to(cfg.levelMax);
    if (j.contains("keepLevelIds")) j["keepLevelIds"].get_to(cfg.keepLevelIds);
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
