#include "PlayLog.h"
#include "DialogActionButtons.h"
#include <API/Il2CppApi.h>

namespace DialogActionButtons {
    Class *get_class() {
        return FindClass("DialogActionButtons");
    }
    Object *get_Instance() {
        Object *val = nullptr;
        IL2Cpp::Il2CppGetStaticFieldValue(OBF("DialogActionButtons"), OBF("Self"), &val);
        return val;
    }
    int GetActionButtonCount() {
        Object *instance = get_Instance();
        if (instance) {
            return instance->invoke_method<int>("GetActionButtonCount");
        }
        return 0;
    }
    Object *GetActionButton(int index) {
        Object *instance = get_Instance();
        if (instance) {
            return instance->invoke_method<Object *>("GetActionButton", index);
        }
        return nullptr;
    }
    void OnClick(int index) {
        void *Self = get_Instance();
        if (Self) {
            int count = GetActionButtonCount();
            if (index >= count) {
                LOGE("DialogActionButtons::OnClick index >= count");
                return;
            }
            Object *ActionButton = GetActionButton(index);
            if (!ActionButton) {
                LOGE("DialogActionButtons::OnClick !ActionButton");
                return;
            }
            ActionButtonInfo::OnPress(ActionButton);
            ActionButtonInfo::OnRelease(ActionButton);
        } else {
            LOGE("DialogActionButtons instance null");
        }
    }
    namespace ActionButtonInfo {
        Class *get_class() {
            return FindClass("DialogActionButtons/ActionButtonInfo");
        }
        void OnPress(Object *instance) {
            if (instance) {
                instance->invoke_method<void>("OnPress");
            }
        }
        void OnRelease(Object *instance) {
            if (instance) {
                instance->invoke_method<void>("OnRelease");
            }
        }
    }
}

