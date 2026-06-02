#include "UILabel.h"
#include <API/Il2CppApi.h>
#include <string>

namespace UILabel {
    Class *get_class() {
        return FindClass("UILabel");
    }
    Array<Object *> *mList() {
        Object *mList = get_class()->get_static_field_value<List<Object *> *>("mList");
        if (!mList) return nullptr;
        Array<Object *> *list = mList->invoke_method<Array<Object *> *>("ToArray");
        return list;
    }
    std::string get_mText(Object *thiz) {
        if (!thiz) return "";
        String *mText = thiz->invoke_method<String *>("get_text");
        if (mText) {
            return mText->to_string();
        }
        return "";
    }
    void set_mText(Object *thiz, String *text) {
        if (!thiz) return;
        thiz->invoke_method<void>("set_text", text);
    }
    Object *get_UILabel() {
        Array<Object *> *list = mList();
        if (list) {
            for (int i = 0; i < list->getLength(); i++) {
                Object *it = list->getPointer()[i];
                if (it) {
                    return it;
                }
            }
        }
        return nullptr;
    }
}

