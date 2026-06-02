#include "PlayLog.h"
//
// Created by PC on 11/10/2025.
//

#include "TableFishingDifficultyImpl.h"
#include <Tools/Tools.h>
namespace TableFishingDifficultyImpl {
    Class *get_class() {
        return FindClass("TableFishingDifficultyImpl");
    }

    Object *thiz = nullptr;

    bool (*old_SerializeCheck)(...);

    bool SerializeCheck(Object *instance, int sid) {
        LOGI("TableFishingDifficultyImpl::SerializeCheck called with sid=%d", sid);
        thiz = instance;
        return old_SerializeCheck(instance, sid);
    }

    void init() {
        Tools::Hook(get_class()->find_method("SerializeCheck", 1)->methodPointer, (void *) SerializeCheck, (void **) &old_SerializeCheck);
    }

    Object *GetTableData(int sid) {
        if (!thiz) {
            LOGE("GetTableData: thiz is null, cannot call GetTableData");
            return nullptr;
        }
        return thiz->invoke_method<Object *>("GetTableData", sid);
    }
}