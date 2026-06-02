#include "FrameWork.h"
#include "SystemHelper.h"

namespace FrameWork {
    Class *get_class() {
        return FindClass("FrameWork");
    }
    Object *get_Instance() {
        return SystemHelper::GetFrameWorkInstance();
    }
    void SystemRestart() {
        Object *instance = get_Instance();
        if (!instance) return;
        instance->invoke_method<void>("SystemRestart");
    }
}
