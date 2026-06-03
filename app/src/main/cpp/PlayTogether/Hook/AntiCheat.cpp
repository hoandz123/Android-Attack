#include "AntiCheat.h"
#include <API/Il2CppApi.h>
#define LOGGER_TAG "ATTACK_PlayTogether"
#include <Includes/Logger.h>
#include <Tools/Tools.h>
#include "kittymemory/MemoryPatch.h"

void AntiCheat::init() {
    LOGD(OBF("AntiCheat::init()"));
    MemoryPatch::createWithHex((uintptr_t)FindClass(OBF("InsectSystem"))->find_method(OBF("CheckInsectTeleport"))->methodPointer, HEX_NOP).Modify();
    MemoryPatch::createWithHex((uintptr_t)FindClass(OBF("AntiCheatListener"))->find_method(OBF("Awake"))->methodPointer, HEX_NOP).Modify();
    MemoryPatch::createWithHex((uintptr_t)FindClass(OBF("AntiCheatListener"))->find_method(OBF("LateUpdate"))->methodPointer, HEX_NOP).Modify();
    MemoryPatch::createWithHex((uintptr_t)FindClass(OBF("ModuleSystem"))->find_method(OBF("CheckCheatDetect"))->methodPointer, HEX_NOP).Modify();
    MemoryPatch::createWithHex((uintptr_t)FindClass(OBF("AntiCheatListener"))->find_method(OBF("get_IsCheating"))->methodPointer, HEX_RET_FALSE).Modify();
    MemoryPatch::createWithHex((uintptr_t)FindClass(OBF("AntiCheatListener"))->find_method(OBF("CheckDetect"), 1)->methodPointer, HEX_RET_FALSE).Modify();
    MemoryPatch::createWithHex((uintptr_t)FindClass(OBF("AntiCheatListener"))->find_method(OBF("OnSpeedHackDetected"))->methodPointer, HEX_RET_FALSE).Modify();
    MemoryPatch::createWithHex((uintptr_t)FindClass(OBF("AntiCheatListener"))->find_method(OBF("OnSpeedHackDetectedGpresto"))->methodPointer, HEX_RET_FALSE).Modify();
    MemoryPatch::createWithHex((uintptr_t)FindClass(OBF("AntiCheatListener"))->find_method(OBF("OnTimeCheatingDetected"))->methodPointer, HEX_RET_FALSE).Modify();
    MemoryPatch::createWithHex((uintptr_t)FindClass(OBF("AntiCheatListener"))->find_method(OBF("OnTableCheatingDetected"), 1)->methodPointer, HEX_RET_FALSE).Modify();
    LOGI(OBF("AntiCheat::init applied"));
}
