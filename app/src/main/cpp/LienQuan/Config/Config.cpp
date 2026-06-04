#include "Config.h"
#include <json/json.hpp>
#include <FileManager.hpp>
#include <Tools/Tools.h>
#include <Includes/obfuscate.h>
#define LOGGER_TAG "ATTACK_LienQuan"
#include <Includes/Logger.h>

namespace lienquan {

LQConfig gLQConfig;

void to_json(nlohmann::json &j, const LQConfig::MainConfig &cfg) {
    j = nlohmann::json{
        {OBF("aimAssist"), cfg.aimAssist},
        {OBF("mapHack"), cfg.mapHack},
    };
}

void from_json(const nlohmann::json &j, LQConfig::MainConfig &cfg) {
    if (j.contains(OBF("aimAssist"))) j[OBF("aimAssist")].get_to(cfg.aimAssist);
    if (j.contains(OBF("mapHack"))) j[OBF("mapHack")].get_to(cfg.mapHack);
}

void to_json(nlohmann::json &j, const LQConfig &cfg) {
    j = nlohmann::json{{OBF("main"), cfg.main}};
}

void from_json(const nlohmann::json &j, LQConfig &cfg) {
    if (j.contains(OBF("main"))) j[OBF("main")].get_to(cfg.main);
}

namespace {

std::string GetConfigFilePath() {
    static std::string path;
    if (!path.empty()) return path;
    std::string packageName = Tools::GetPackageName();
    if (packageName.empty()) packageName = OBF("com.garena.game.kgvn");
    path = OBF("/storage/emulated/0/Android/data/") + packageName + OBF("/lq_config.json");
    return path;
}

} // namespace

void LQConfig::Load(const std::string &content) {
    try {
        auto j = nlohmann::json::parse(content);
        from_json(j, gLQConfig);
    } catch (const std::exception &e) {
        LOGE(OBF("LQConfig::Load: %s"), e.what());
    }
}

std::string LQConfig::GetConfigContent() {
    try {
        nlohmann::json j;
        to_json(j, gLQConfig);
        return j.dump(4);
    } catch (...) {
        return "{}";
    }
}

bool SaveConfig() {
    try {
        std::string content = gLQConfig.GetConfigContent();
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
        gLQConfig.Load(content);
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace lienquan
