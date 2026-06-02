#ifndef SDK_JSONCONVERT_H
#define SDK_JSONCONVERT_H

#include <API/Il2CppApi.h>

namespace JsonConvert {
    Class *get_class();
    String *SerializeObject(Object *obj);
}

#endif // SDK_JSONCONVERT_H

