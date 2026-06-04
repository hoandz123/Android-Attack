//
// Created by HMGTEAM on 11/05/2025.
//

#include "Object.h"
#include "API/Il2CppApi.h"

bool UnityEngine::Object::isValid() {
    return this->m_CachedPtr != nullptr;
}

bool UnityEngine::Object::IsNativeObjectAlive(Object* obj) {
    static bool (*IsNativeObjectAlive)(Object* obj) = (bool (*)(Object*)) GET_METHOD(
        "UnityEngine.CoreModule.dll", "UnityEngine", "Object", "IsNativeObjectAlive", 1);
    if (IsNativeObjectAlive) {
        return IsNativeObjectAlive(obj);
    }
    return false;
}
Array<void **> *UnityEngine::Object::FindObjectsOfTypeAll(void *type) {
    static Array<void **> * (*FindObjectsOfTypeAll)(void* type) = (Array<void **> * (*)(void*)) GET_METHOD("UnityEngine", "Object", "FindObjectsOfType", 1);
    return FindObjectsOfTypeAll(type);
}

