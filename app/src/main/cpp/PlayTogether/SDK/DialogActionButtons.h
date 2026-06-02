#ifndef SDK_DIALOGACTIONBUTTONS_H
#define SDK_DIALOGACTIONBUTTONS_H

#include <API/Il2CppApi.h>

namespace DialogActionButtons {
    Class *get_class();
    Object *get_Instance();
    int GetActionButtonCount();
    Object *GetActionButton(int index = 0);
    void OnClick(int index = 0);
    namespace ActionButtonInfo {
        Class *get_class();
        void OnPress(Object *instance);
        void OnRelease(Object *instance);
    }
}

#endif // SDK_DIALOGACTIONBUTTONS_H

