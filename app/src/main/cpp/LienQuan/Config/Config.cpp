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
        {OBF("mapHack"), cfg.mapHack},
        {OBF("blockDlcDownload"), cfg.blockDlcDownload},
        {OBF("unlockSkin"), cfg.unlockSkin},
        {OBF("unlockButton"), cfg.unlockButton},
        {OBF("buttonHeroId"), cfg.buttonHeroId},
        {OBF("buttonSkinId"), cfg.buttonSkinId},
    };
}

void from_json(const nlohmann::json &j, LQConfig::MainConfig &cfg) {
    if (j.contains(OBF("mapHack"))) j[OBF("mapHack")].get_to(cfg.mapHack);
    if (j.contains(OBF("blockDlcDownload"))) j[OBF("blockDlcDownload")].get_to(cfg.blockDlcDownload);
    if (j.contains(OBF("unlockSkin"))) j[OBF("unlockSkin")].get_to(cfg.unlockSkin);
    if (j.contains(OBF("unlockButton"))) j[OBF("unlockButton")].get_to(cfg.unlockButton);
    if (j.contains(OBF("buttonHeroId"))) j[OBF("buttonHeroId")].get_to(cfg.buttonHeroId);
    if (j.contains(OBF("buttonSkinId"))) j[OBF("buttonSkinId")].get_to(cfg.buttonSkinId);
}

void to_json(nlohmann::json &j, const LQConfig::EspConfig &cfg) {
    j = nlohmann::json{
        {OBF("enabled"), cfg.enabled},
        {OBF("enemiesOnly"), cfg.enemiesOnly},
        {OBF("lineThickness"), cfg.lineThickness},
        {OBF("lineColor"), nlohmann::json{cfg.lineColor[0], cfg.lineColor[1], cfg.lineColor[2], cfg.lineColor[3]}},
        {OBF("showHeroIcons"), cfg.showHeroIcons},
        {OBF("miniMapIconSize"), cfg.miniMapIconSize},
        {OBF("iconBorderColor"), nlohmann::json{cfg.iconBorderColor[0], cfg.iconBorderColor[1], cfg.iconBorderColor[2], cfg.iconBorderColor[3]}},
        {OBF("iconShadowColor"), nlohmann::json{cfg.iconShadowColor[0], cfg.iconShadowColor[1], cfg.iconShadowColor[2], cfg.iconShadowColor[3]}},
    };
}

void from_json(const nlohmann::json &j, LQConfig::EspConfig &cfg) {
    if (j.contains(OBF("enabled"))) j[OBF("enabled")].get_to(cfg.enabled);
    if (j.contains(OBF("enemiesOnly"))) j[OBF("enemiesOnly")].get_to(cfg.enemiesOnly);
    if (j.contains(OBF("lineThickness"))) j[OBF("lineThickness")].get_to(cfg.lineThickness);
    if (j.contains(OBF("lineColor"))) {
        const auto &a = j[OBF("lineColor")];
        if (a.is_array() && a.size() >= 4)
            for (int i = 0; i < 4; ++i) cfg.lineColor[i] = a[i].get<float>();
    }
    if (j.contains(OBF("showHeroIcons"))) j[OBF("showHeroIcons")].get_to(cfg.showHeroIcons);
    if (j.contains(OBF("miniMapIconSize"))) j[OBF("miniMapIconSize")].get_to(cfg.miniMapIconSize);
    if (j.contains(OBF("iconBorderColor"))) {
        const auto &a = j[OBF("iconBorderColor")];
        if (a.is_array() && a.size() >= 4)
            for (int i = 0; i < 4; ++i) cfg.iconBorderColor[i] = a[i].get<float>();
    }
    if (j.contains(OBF("iconShadowColor"))) {
        const auto &a = j[OBF("iconShadowColor")];
        if (a.is_array() && a.size() >= 4)
            for (int i = 0; i < 4; ++i) cfg.iconShadowColor[i] = a[i].get<float>();
    }
}

void to_json(nlohmann::json &j, const LQConfig &cfg) {
    j = nlohmann::json{{OBF("main"), cfg.main}, {OBF("esp"), cfg.esp}};
}

void from_json(const nlohmann::json &j, LQConfig &cfg) {
    if (j.contains(OBF("main"))) j[OBF("main")].get_to(cfg.main);
    if (j.contains(OBF("esp"))) j[OBF("esp")].get_to(cfg.esp);
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
