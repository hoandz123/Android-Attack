//
// Created by HMGTEAM on 11/05/2025.
//
#pragma once

#include "iostream"
#include "API/Il2CppApi.h"


namespace System {
    class Object : public Il2CppObject {
    public:
        std::string ToString() {
            String* value = invoke_method<String*>("ToString");
            return value ? value->to_string() : "";
        }
    };
}
