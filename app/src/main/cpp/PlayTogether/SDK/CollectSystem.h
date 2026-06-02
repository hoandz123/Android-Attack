#ifndef SDK_COLLECTSYSTEM_H
#define SDK_COLLECTSYSTEM_H

#include <API/Il2CppApi.h>

namespace CollectSystem {
    Class *get_class();
    Object *get_Instance();
    List<Object *> *_mapObjectInfoList();
    List<Object *> *_fieldMonsterInfoList();

    void Update();
}

#endif // SDK_COLLECTSYSTEM_H

