#include "DialogBaseLock.h"
#include <Tools/Tools.h>
#define LOGGER_TAG "ATTACK_DialogBaseLock"
#include <Includes/Logger.h>

namespace DialogBaseLock {

bool on_get_IsLock(Object * /*self*/) { return false; }

void onLock(Object * /*self*/, int /*protocolOrType*/, bool /*duplicate*/, float /*autoUnlock*/) {}

void onCheckLock(Object * /*self*/) {}

void onDialogShow(Object * /*self*/) {}

void installHooks() {
    Tools::Hook((void *) GET_METHOD("DialogBaseLock", "get_IsLock", 0), (void *) on_get_IsLock, nullptr);
    Tools::Hook((void *) GET_METHOD("Assembly-CSharp.dll", "", "DialogBaseLock", "Lock", 3, 1), (void *) onLock, nullptr);
    Tools::Hook((void *) GET_METHOD("Assembly-CSharp.dll", "", "DialogBaseLock", "Lock", 3, 2), (void *) onLock, nullptr);
    Tools::Hook((void *) GET_METHOD("Assembly-CSharp.dll", "", "DialogBaseLock", "Lock", 3, 3), (void *) onLock, nullptr);
    Tools::Hook((void *) GET_METHOD("DialogBaseLock", "CheckLock", 0), (void *) onCheckLock, nullptr);
    Tools::Hook((void *) GET_METHOD("DialogBaseLock", "DialogShow", 0), (void *) onDialogShow, nullptr);
    LOGI(OBF("DialogBaseLock hooks installed (no network UI lock)"));
}

}
