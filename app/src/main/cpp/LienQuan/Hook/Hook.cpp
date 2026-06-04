#include "Hook.h"
#include "Bypass/AntiCheat.h"
#include "MapBright.h"
#include "../Config/Config.h"
#define LOGGER_TAG "ATTACK_LienQuan"
#include <Includes/Logger.h>
#include <Includes/obfuscate.h>
#include <atomic>

namespace lienquan {
namespace Hook {

namespace {
std::atomic<bool> s_initOnce{false};
}

void init() {
    bool expected = false;
    if (!s_initOnce.compare_exchange_strong(expected, true)) {
        LOGI(OBF("[LienQuan] Hook init skipped (already done)"));
        return;
    }
    LoadConfig();
    AntiCheat::init();
    MapBright::InstallHooks();
    LOGI(OBF("[LienQuan] Hook init done"));
}

} // namespace Hook
} // namespace lienquan
