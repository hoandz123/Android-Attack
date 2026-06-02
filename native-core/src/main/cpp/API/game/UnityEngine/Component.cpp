//
// Created by HMGTEAM on 11/05/2025.
//

#include "Component.h"

#include "API/Il2CppApi.h"
#include "UnityEngine/Transform.h"
#include "UnityEngine/Object.h"

using namespace UnityEngine;

Transform* Component::get_transform() {
    return ((Il2CppObject*)this)->invoke_method<Transform *>("get_transform");
}
void *Component::GetType(String *typeName) {
    static void* (*GetType)(void* typeName) = (void* (*)(void*)) GET_METHOD("System", "Type", "GetType", 1);
    return GetType(typeName);
}
