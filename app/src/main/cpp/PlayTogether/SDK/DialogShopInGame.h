//
// Created by TEAMHMG on 14/09/2025.
//

#pragma once
#ifndef IL2CPP_PLAY_DIALOGSHOPINGAME_H
#define IL2CPP_PLAY_DIALOGSHOPINGAME_H
#include <API/Il2CppApi.h>

namespace DialogShopInGame {
    Class* get_class();
    Object* get_Instance();

    extern void (*old_Update)(Object *);
    void Update(Object *instance);
}

#endif //IL2CPP_PLAY_DIALOGSHOPINGAME_H
