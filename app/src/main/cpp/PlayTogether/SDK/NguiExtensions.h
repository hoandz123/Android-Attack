#ifndef SDK_NGUIEXTENSIONS_H
#define SDK_NGUIEXTENSIONS_H

#include <string>
#include <API/Il2CppApi.h>

namespace NguiExtensions {
    Class *get_class();
    void SetLabelReplacelineToSpace(Object *label, int index);
    std::string GetNameIDText(int nameID);
}

#endif // SDK_NGUIEXTENSIONS_H

