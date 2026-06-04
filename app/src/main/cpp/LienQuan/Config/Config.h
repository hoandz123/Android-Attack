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
        bool showDebug = false;
        bool minimapDot = false;
        float minimapDotRadius = 3.f;
        bool showHeroIcons = false;
    } esp;

    void Load(const std::string &content);
    std::string GetConfigContent();
};

extern LQConfig gLQConfig;

bool SaveConfig();
bool LoadConfig();

} // namespace lienquan
