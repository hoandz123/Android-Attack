#ifndef SDK_DIALOGFISHINGGETITEM_H
#define SDK_DIALOGFISHINGGETITEM_H

#include <API/Il2CppApi.h>

namespace DialogFishingGetItem {
    Class *get_class();
    extern void (*old_OnItemSell)(Object *thiz, bool success);
    void OnItemSell(Object *thiz, bool success);
}

#endif // SDK_DIALOGFISHINGGETITEM_H

