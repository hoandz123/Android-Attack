//
// Created by PC on 08/10/2025.
//

#ifndef IL2CPP_PLAY_EVENTPICKUPITEMMANAGER_H
#define IL2CPP_PLAY_EVENTPICKUPITEMMANAGER_H
#include "API/Il2CppApi.h"

namespace EventPickUpItemManager {
    Class* get_class();
    Object* get_instance();
    List<Object*>* get_allPickupItems();

    void init();
}

#endif //IL2CPP_PLAY_EVENTPICKUPITEMMANAGER_H
