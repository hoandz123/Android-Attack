#include "PlayLog.h"
#include "DialogJoyStick.h"
#include <API/Il2CppApi.h>
#include <API/Vector2.h>

namespace DialogJoyStick {
    Class *get_class() {
        return FindClass("DialogJoyStick");
    }
    Object *get_Instance() {
        Object *val = nullptr;
        IL2Cpp::Il2CppGetStaticFieldValue(OBF("DialogJoyStick"), OBF("Self"), &val);
        return val;
    }
    void OnPress_JumpButton() {
        Object *instance = get_Instance();
        if (instance) {
            instance->invoke_method<void>("OnPress_JumpButton");
            return;
        }
        LOGE("DialogJoyStick::OnPress_JumpButton instance null");
    }
    void OnPress_ActionButton() {
        Object *instance = get_Instance();
        if (instance) {
            instance->invoke_method<void>("OnPress_ActionButton");
            return;
        }
        LOGE("DialogJoyStick::OnPress_ActionButton instance null");
    }
    void OnMoveJoyStickEvent(Vector2 delta) {
        Object *instance = get_Instance();
        if (instance) {
            instance->invoke_method<void>("OnMoveJoyStickEvent", delta);
        }
    }
}

