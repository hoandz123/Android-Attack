#pragma once

#include <string>

namespace lienquan {

struct LQConfig {
    struct MainConfig {
        bool aimAssist = false;
        bool mapHack = false;
    } main;

    void Load(const std::string &content);
    std::string GetConfigContent();
};

extern LQConfig gLQConfig;

bool SaveConfig();
bool LoadConfig();

} // namespace lienquan
