//
// Created by PC on 07/10/2025.
//

#ifndef IL2CPP_PLAY_MINIGAMETOWEROFHELL_H
#define IL2CPP_PLAY_MINIGAMETOWEROFHELL_H
#include "API/Il2CppApi.h"


namespace MiniGameTowerOfHell {
    Class *get_class();
    void init();

    extern void (*old_Update)(Object *instance);
    void Update(Object *instance);
}


#endif //IL2CPP_PLAY_MINIGAMETOWEROFHELL_H
