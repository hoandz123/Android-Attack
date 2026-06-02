#include "FrameWork.h"
#include "SystemHelper.h"

namespace FrameWork {
    Class *get_class() {
        return FindClass("FrameWork");
    }
    Object *get_Instance() {
        return SystemHelper::GetFrameWorkInstance();
    }
}
