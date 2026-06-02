#include "SystemHelper.h"

namespace SystemHelper {
    Class *get_class() {
        return FindClass("SystemHelper");
    }
    Object *GetFrameWorkInstance() {
        return get_class()->find_method("GetFrameWorkInstance", 0)->static_invoke<Object *>();
    }
}
