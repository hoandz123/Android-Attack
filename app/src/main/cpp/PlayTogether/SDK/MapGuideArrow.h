//
// Created by TEAMHMG on 13/09/2025.
//

#pragma once
#ifndef IL2CPP_PLAY_MAPGUIDEARROW_H
#define IL2CPP_PLAY_MAPGUIDEARROW_H
#include <API/Il2CppApi.h>

namespace MapGuideArrow {
    Class *get_class();

    extern void (*old_Update)(Object *instance);
    void Update(Object *instance);
}

#endif //IL2CPP_PLAY_MAPGUIDEARROW_H
