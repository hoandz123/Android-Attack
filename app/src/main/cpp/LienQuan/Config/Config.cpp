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
        {OBF("lineThickness"), cfg.lineThickness},
        {OBF("lineColor"), nlohmann::json{cfg.lineColor[0], cfg.lineColor[1], cfg.lineColor[2], cfg.lineColor[3]}},
        {OBF("showHeroIcons"), cfg.showHeroIcons},
        {OBF("miniMapIconSize"), cfg.miniMapIconSize},
        {OBF("iconBorderColor"), nlohmann::json{cfg.iconBorderColor[0], cfg.iconBorderColor[1], cfg.iconBorderColor[2], cfg.iconBorderColor[3]}},
        {OBF("iconShadowColor"), nlohmann::json{cfg.iconShadowColor[0], cfg.iconShadowColor[1], cfg.iconShadowColor[2], cfg.iconShadowColor[3]}},
        {OBF("showInfo"), cfg.showInfo},
        {OBF("infoColor"), nlohmann::json{cfg.infoColor[0], cfg.infoColor[1], cfg.infoColor[2], cfg.infoColor[3]}},
        {OBF("showHpBar"), cfg.showHpBar},
        {OBF("showDistance"), cfg.showDistance},
        {OBF("lowHpHighlight"), cfg.lowHpHighlight},
        {OBF("offscreenArrow"), cfg.offscreenArrow},
        {OBF("hpBarWidth"), cfg.hpBarWidth},
        {OBF("hpBarHeight"), cfg.hpBarHeight},
        {OBF("hpBarOffsetX"), cfg.hpBarOffsetX},
        {OBF("hpBarOffsetY"), cfg.hpBarOffsetY},
        {OBF("infoOffsetX"), cfg.infoOffsetX},
        {OBF("infoOffsetY"), cfg.infoOffsetY},
        {OBF("infoLayout"), cfg.infoLayout},
        {OBF("arrowSize"), cfg.arrowSize},
        {OBF("arrowMargin"), cfg.arrowMargin},
        {OBF("showCooldowns"), cfg.showCooldowns},
        {OBF("cooldownDotSize"), cfg.cooldownDotSize},
        {OBF("cooldownTextSize"), cfg.cooldownTextSize},
        {OBF("cooldownSpacing"), cfg.cooldownSpacing},
        {OBF("cooldownOffsetX"), cfg.cooldownOffsetX},
        {OBF("cooldownOffsetY"), cfg.cooldownOffsetY},
    };
}

void from_json(const nlohmann::json &j, LQConfig::EspConfig &cfg) {
    if (j.contains(OBF("enabled"))) j[OBF("enabled")].get_to(cfg.enabled);
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
    if (j.contains(OBF("showInfo"))) j[OBF("showInfo")].get_to(cfg.showInfo);
    if (j.contains(OBF("infoColor"))) {
        const auto &a = j[OBF("infoColor")];
        if (a.is_array() && a.size() >= 4)
            for (int i = 0; i < 4; ++i) cfg.infoColor[i] = a[i].get<float>();
    }
    if (j.contains(OBF("showHpBar"))) j[OBF("showHpBar")].get_to(cfg.showHpBar);
    if (j.contains(OBF("showDistance"))) j[OBF("showDistance")].get_to(cfg.showDistance);
    if (j.contains(OBF("lowHpHighlight"))) j[OBF("lowHpHighlight")].get_to(cfg.lowHpHighlight);
    if (j.contains(OBF("offscreenArrow"))) j[OBF("offscreenArrow")].get_to(cfg.offscreenArrow);
    if (j.contains(OBF("hpBarWidth"))) j[OBF("hpBarWidth")].get_to(cfg.hpBarWidth);
    if (j.contains(OBF("hpBarHeight"))) j[OBF("hpBarHeight")].get_to(cfg.hpBarHeight);
    if (j.contains(OBF("hpBarOffsetX"))) j[OBF("hpBarOffsetX")].get_to(cfg.hpBarOffsetX);
    if (j.contains(OBF("hpBarOffsetY"))) j[OBF("hpBarOffsetY")].get_to(cfg.hpBarOffsetY);
    if (j.contains(OBF("infoOffsetX"))) j[OBF("infoOffsetX")].get_to(cfg.infoOffsetX);
    if (j.contains(OBF("infoOffsetY"))) j[OBF("infoOffsetY")].get_to(cfg.infoOffsetY);
    if (j.contains(OBF("infoLayout"))) j[OBF("infoLayout")].get_to(cfg.infoLayout);
    if (j.contains(OBF("arrowSize"))) j[OBF("arrowSize")].get_to(cfg.arrowSize);
    if (j.contains(OBF("arrowMargin"))) j[OBF("arrowMargin")].get_to(cfg.arrowMargin);
    if (j.contains(OBF("showCooldowns"))) j[OBF("showCooldowns")].get_to(cfg.showCooldowns);
    if (j.contains(OBF("cooldownDotSize"))) j[OBF("cooldownDotSize")].get_to(cfg.cooldownDotSize);
    if (j.contains(OBF("cooldownTextSize"))) j[OBF("cooldownTextSize")].get_to(cfg.cooldownTextSize);
    if (j.contains(OBF("cooldownSpacing"))) j[OBF("cooldownSpacing")].get_to(cfg.cooldownSpacing);
    if (j.contains(OBF("cooldownOffsetX"))) j[OBF("cooldownOffsetX")].get_to(cfg.cooldownOffsetX);
    if (j.contains(OBF("cooldownOffsetY"))) j[OBF("cooldownOffsetY")].get_to(cfg.cooldownOffsetY);
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
