//
// Created by TEAMHMG on 05/09/2025.
//

#pragma once
#ifndef PLAY_IL2CPP_DELEGATE_H
#define PLAY_IL2CPP_DELEGATE_H

#include "Object.h"
#include "IntPtr.h"

namespace System {
    class Delegate : public System::Object {
    public:
        System::IntPtr method_ptr;
        System::IntPtr invoke_impl;
        System::Object* m_target;
        System::IntPtr method;
        System::IntPtr delegate_trampoline;
        System::IntPtr extra_arg;
        System::IntPtr method_code;
        System::IntPtr interp_method;
        System::IntPtr interp_invoke_impl;
        System::Object *method_info;
        System::Object *original_method_info;
        System::DelegateData *data;
        bool method_is_virtual;

        System::Object *get_Target() {
            static auto _ = (System::Object *(*)(System::Delegate *)) GET_METHOD("Delegate", "get_Target", 0);
            return _(this);
        }
    };
}

#endif //PLAY_IL2CPP_DELEGATE_H
