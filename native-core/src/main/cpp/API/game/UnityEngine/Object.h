//
// Created by HMGTEAM on 11/05/2025.
//
#pragma once
#ifndef PLAY_PRO_MAX_UNITYENGINE_OBJECT_H
#define PLAY_PRO_MAX_UNITYENGINE_OBJECT_H
#include "API/Il2CppApi.h"
#include <System/Object.h>
#include <System/IntPtr.h>

namespace UnityEngine {
    class Object : public System::Object {
    public:
        System::IntPtr* m_CachedPtr;

        static bool IsNativeObjectAlive(Object* obj);
        static Array<void **> * FindObjectsOfTypeAll(void* type);

        bool isValid();
    };
}


#endif //PLAY_PRO_MAX_UNITYENGINE_OBJECT_H
