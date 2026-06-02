//
// Created by TEAMHMG on 13/09/2025.
//

#pragma once
#ifndef IL2CPP_PLAY_HEADUPSELECTBUTTON_H
#define IL2CPP_PLAY_HEADUPSELECTBUTTON_H
#include <API/Il2CppApi.h>

namespace HeadUpSelectButton {
    Class *get_class();
    extern void (*old_SetSprite)(Object *instance, String *spriteName);
    void SetSprite(Object *instance, String *spriteName);

    extern void (*old_UpdatePosition)(Object *instance);
    void UpdatePosition(Object *instance);
}

#endif //IL2CPP_PLAY_HEADUPSELECTBUTTON_H
