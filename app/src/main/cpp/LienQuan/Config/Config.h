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
        bool enemiesOnly = true;
        float lineThickness = 2.f;
        float lineColor[4] = {1.f, 60.f / 255.f, 60.f / 255.f, 220.f / 255.f};
        bool showHeroIcons = false;
        float miniMapIconSize = 20.f;
        float iconBorderColor[4] = {1.f, 1.f, 1.f, 190.f / 255.f};
        float iconShadowColor[4] = {20.f / 255.f, 20.f / 255.f, 20.f / 255.f, 210.f / 255.f};
    } esp;

    void Load(const std::string &content);
    std::string GetConfigContent();
};

extern LQConfig gLQConfig;

bool SaveConfig();
bool LoadConfig();

} // namespace lienquan
