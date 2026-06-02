//
// Created by HMGTEAM on 23/05/2025.
//

#include "Application.h"
#include "API/Il2CppApi.h"

void Application::OpenURL(std::string url) {
    static auto _ = (void (*)(String*)) GET_METHOD("Application", "OpenURL", 1);
    _(String::Create(url.c_str()));
}

int Application::get_targetFrameRate() {
    static auto _ = (int (*)()) GET_METHOD("Application", "get_targetFrameRate", 0);
    return _();
}

void Application::set_targetFrameRate(int value) {
    static auto _ = (void (*)(int)) GET_METHOD("Application", "set_targetFrameRate", 1);
    _(value);
}
