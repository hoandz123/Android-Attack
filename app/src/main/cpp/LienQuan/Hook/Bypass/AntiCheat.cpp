#include "AntiCheat.h"
#include "anogs_bypass.hpp"
#include "il2cpp_bypass.hpp"
#define LOGGER_TAG "ATTACK_LienQuan"
#include <Includes/Logger.h>
#include <Includes/obfuscate.h>
#include <thread>

namespace lienquan {

void AntiCheat::init() {
    std::thread([]() { anogs_bypass::apply_anogs_patches(); }).detach();

    il2cpp_bypass::install();


    LOGI(OBF("[LienQuan] AntiCheat bypass started"));
}

} // namespace lienquan
