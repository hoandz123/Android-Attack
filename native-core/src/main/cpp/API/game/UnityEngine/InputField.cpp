//
// Created by TEAMHMG on 19/07/2025.
//

#include "InputField.h"

String *UnityEngine::InputField::get_clipboard() {
    static auto _ = (String * (*)()) GET_METHOD("UnityEngine.UI.dll", "UnityEngine.UI", "InputField", "get_clipboard", 0);
    return _();
}
void UnityEngine::InputField::set_clipboard(std::string value) {
    static auto _ = (void (*)(String *)) GET_METHOD("UnityEngine.UI.dll", "UnityEngine.UI", "InputField", "set_clipboard", 1);
    _ (String::Create(value.c_str()));
}
