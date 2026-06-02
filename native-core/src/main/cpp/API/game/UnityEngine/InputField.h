//
// Created by TEAMHMG on 19/07/2025.
//

#pragma once
#ifndef PLAY_PRO_MAX_V2_INPUTFIELD_H
#define PLAY_PRO_MAX_V2_INPUTFIELD_H

#include "Object.h"
#include "API/Il2CppApi.h"

namespace UnityEngine {
    class InputField : public System::Object {
    public:
        static String* get_clipboard();
        static void set_clipboard(std::string value);
    };
}

#endif //PLAY_PRO_MAX_V2_INPUTFIELD_H
