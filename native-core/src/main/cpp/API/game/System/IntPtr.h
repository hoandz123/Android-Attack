//
// Created by HMGTEAM on 11/05/2025.
//

#pragma once
#include "Object.h"

namespace System {
    struct IntPtr {
    public:
        void* m_value;

        IntPtr() : m_value(nullptr) {}

        IntPtr(void* value) : m_value(value) {}

        void* ToPointer() {
            return m_value;
        }

        static IntPtr Zero() {
            return IntPtr(nullptr);
        }
    };
}
