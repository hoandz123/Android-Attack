#include "ContentSystem.h"

namespace ContentSystem {

Class *get_class() {
    return FindClass(OBF("ContentSystem"));
}

Object *get_Combine() {
    Object *combine = nullptr;
    IL2Cpp::Il2CppGetStaticFieldValue(OBF("ContentSystem"), OBF("Combine"), &combine);
    return combine;
}

}
