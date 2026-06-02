//
// Created by PC on 07/10/2025.
//

#ifndef IL2CPP_PLAY_MINIGAMEOBBY_H
#define IL2CPP_PLAY_MINIGAMEOBBY_H
#include "API/Il2CppApi.h"

namespace MiniGameObby {
    Class* get_class();
    void init();

    extern void (*old_Update)(...);
    void Update(Object* instance);
}


#endif //IL2CPP_PLAY_MINIGAMEOBBY_H
