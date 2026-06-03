#pragma once

#include <API/Il2CppApi.h>

namespace DialogBaseLock {

void installHooks();

bool on_get_IsLock(Object *self);
void onLock(Object *self, int protocolOrType, bool duplicate, float autoUnlock);
void onCheckLock(Object *self);
void onDialogShow(Object *self);

}
