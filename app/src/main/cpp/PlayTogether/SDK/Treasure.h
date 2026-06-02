//
// Created by TEAMHMG on 14/09/2025.
//

#pragma once
#ifndef IL2CPP_PLAY_TREASURE_H
#define IL2CPP_PLAY_TREASURE_H
#include <API/Il2CppApi.h>

namespace Treasure {
    Class* get_class();

    extern void (*old_Update)(Object* instance);
    void Update(Object* instance);
}

#endif //IL2CPP_PLAY_TREASURE_H
