//
// Created by PC on 28/10/2025.
//

#pragma once

#include <utility>
namespace System {
    template<typename T>
    struct Nullable {
        bool hasValue;
        T value;

        Nullable() : hasValue(false), value() {}

        Nullable(const T &v) : hasValue(true), value(v) {}

        Nullable(T &&v) : hasValue(true), value(std::move(v)) {}

        void Reset() {
            hasValue = false;
            value = T();
        }

        bool HasValue() const { return hasValue; }

        const T &Value() const { return value; }

        T &Value() { return value; }
    };
}