#pragma once

#include <string>

namespace lienquan {

struct LQConfig {
    struct MainConfig {
        bool mapHack = false;
        bool blockDlcDownload = false;
        bool unlockSkin = false;
        bool unlockButton = false;
        int buttonHeroId = 0;
        int buttonSkinId = 0;
    } main;

    struct EspConfig {
        bool enabled = true;
        float lineThickness = 2.f;
        float lineColor[4] = {1.f, 60.f / 255.f, 60.f / 255.f, 220.f / 255.f};
        bool showHeroIcons = false;
        float miniMapIconSize = 20.f;
        float iconBorderColor[4] = {1.f, 1.f, 1.f, 190.f / 255.f};
        float iconShadowColor[4] = {20.f / 255.f, 20.f / 255.f, 20.f / 255.f, 210.f / 255.f};
        bool showInfo = true;
        float infoColor[4] = {1.f, 1.f, 1.f, 230.f / 255.f};
        bool showHpBar = true;
        bool showDistance = true;
        bool lowHpHighlight = true;
        bool offscreenArrow = true;
        float hpBarWidth = 80.f;
        float hpBarHeight = 8.f;
        float hpBarOffsetX = 0.f;
        float hpBarOffsetY = 2.f;
        float infoOffsetX = 6.f;
        float infoOffsetY = -4.f;
        int infoLayout = 0;
        float arrowSize = 14.f;
        float arrowMargin = 36.f;
        bool showCooldowns = true;
        float cooldownDotSize = 5.f;
        float cooldownTextSize = 11.f;
        float cooldownSpacing = 16.f;
        float cooldownOffsetX = 0.f;
        float cooldownOffsetY = 4.f;
    } esp;

    void Load(const std::string &content);
    std::string GetConfigContent();
};

extern LQConfig gLQConfig;

bool SaveConfig();
bool LoadConfig();

} // namespace lienquan
