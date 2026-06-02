//
// Created by HMGTEAM on 11/05/2025.
//

#ifndef PLAY_PRO_MAX_COMPONENT_H
#define PLAY_PRO_MAX_COMPONENT_H
#include "Object.h"


namespace UnityEngine {
    class Transform;
    class Component : public Object {
    public:
        Transform* get_transform();
        static void* GetType(String* typeName);

    };
}


#endif //PLAY_PRO_MAX_COMPONENT_H
