#ifndef SDK_DIALOGJOYSTICK_H
#define SDK_DIALOGJOYSTICK_H

#include <API/Il2CppApi.h>
#include <API/Vector2.h>

namespace DialogJoyStick {
    Class *get_class();
    Object *get_Instance();
    void OnPress_JumpButton();
    void OnPress_ActionButton();
    void OnMoveJoyStickEvent(Vector2 delta);
}

#endif // SDK_DIALOGJOYSTICK_H

