//
// Created by TEAMHMG on 18/07/2025.
//

#include "Screen.h"
#include "API/Il2CppApi.h"

namespace UnityEngine {
    int Screen::get_width() {
        static auto _ = (int (*)()) GET_METHOD("UnityEngine.CoreModule.dll", "UnityEngine", "Screen", "get_width", 0);
        return _();
    }
    int Screen::get_height() {
        static auto _ = (int (*)()) GET_METHOD("UnityEngine.CoreModule.dll", "UnityEngine", "Screen", "get_height", 0);
        return _();
    }
    float Screen::get_dpi() {
        static auto _ = (float (*)()) GET_METHOD("UnityEngine.CoreModule.dll", "UnityEngine", "Screen", "get_dpi", 0);
        return _();
    }
    void Screen::SetResolution(int width, int height, bool fullscreen, int preferredRefreshRate) {
        static auto _ = (void (*)(int, int, bool, int)) GET_METHOD("UnityEngine.CoreModule.dll", "UnityEngine", "Screen", "SetResolution", 4);
        _(width, height, fullscreen, preferredRefreshRate);
    }
}