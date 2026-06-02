#include "AntiCheat.h"
#include <API/Il2CppApi.h>
#include <Includes/obfuscate.h>
#define LOG_TAG OBF("AttackPlugin")
#include <Includes/Logger.h>
#include <Tools/Tools.h>
#include "kittymemory/MemoryPatch.h"

void AntiCheat::init() {
    LOGD(OBF("AntiCheat::init()"));
    MemoryPatch::createWithHex((uintptr_t)FindClass("InsectSystem")->find_method("CheckInsectTeleport")->methodPointer, HEX_NOP).Modify();
    MemoryPatch::createWithHex((uintptr_t)FindClass("AntiCheatListener")->find_method("Awake")->methodPointer, HEX_NOP).Modify();
    MemoryPatch::createWithHex((uintptr_t)FindClass("AntiCheatListener")->find_method("LateUpdate")->methodPointer, HEX_NOP).Modify();
    MemoryPatch::createWithHex((uintptr_t)FindClass("ModuleSystem")->find_method("CheckCheatDetect")->methodPointer, HEX_NOP).Modify();
    MemoryPatch::createWithHex((uintptr_t)FindClass("AntiCheatListener")->find_method("get_IsCheating")->methodPointer, HEX_RET_FALSE).Modify();
    MemoryPatch::createWithHex((uintptr_t)FindClass("AntiCheatListener")->find_method("CheckDetect", 1)->methodPointer, HEX_RET_FALSE).Modify();
    MemoryPatch::createWithHex((uintptr_t)FindClass("AntiCheatListener")->find_method("OnSpeedHackDetected")->methodPointer, HEX_RET_FALSE).Modify();
    MemoryPatch::createWithHex((uintptr_t)FindClass("AntiCheatListener")->find_method("OnSpeedHackDetectedGpresto")->methodPointer, HEX_RET_FALSE).Modify();
    MemoryPatch::createWithHex((uintptr_t)FindClass("AntiCheatListener")->find_method("OnTimeCheatingDetected")->methodPointer, HEX_RET_FALSE).Modify();
    MemoryPatch::createWithHex((uintptr_t)FindClass("AntiCheatListener")->find_method("OnTableCheatingDetected", 1)->methodPointer, HEX_RET_FALSE).Modify();
    LOGI(OBF("AntiCheat::init applied"));
}
