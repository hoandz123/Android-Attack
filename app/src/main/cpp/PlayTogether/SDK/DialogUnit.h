#ifndef SDK_DIALOGUNIT_H
#define SDK_DIALOGUNIT_H

#include <API/Il2CppApi.h>

namespace DialogUnit {
    Class *get_class();
    extern void (*old_DialogShow)(Object *thiz);
    void DialogShow(Object *thiz);
}

#endif // SDK_DIALOGUNIT_H

