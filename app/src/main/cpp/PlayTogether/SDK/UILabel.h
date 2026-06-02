#ifndef SDK_UILABEL_H
#define SDK_UILABEL_H

#include <API/Il2CppApi.h>

namespace UILabel {
    Class *get_class();
    Array<Object *> *mList();
    std::string get_mText(Object *thiz);
    void set_mText(Object *thiz, String *text);
    Object *get_UILabel();
}

#endif // SDK_UILABEL_H

