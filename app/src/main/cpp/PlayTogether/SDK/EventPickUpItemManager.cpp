//
// Created by PC on 08/10/2025.
//

#include "EventPickUpItemManager.h"
#include "Tools/Tools.h"
#include "PlayLog.h"

namespace EventPickUpItemManager {
    Class *get_class() {
        return FindClass("EventPickUpItemManager");
    }

    Object *instance = nullptr;

    Object *get_instance() {
        return instance;
    }
    List<Object *> *get_allPickupItems() {
        if (!instance) return nullptr;
        return instance->get_field_object<List<Object *> *>("_allPickupItems");
    }

    void (*old_Start)(...);

    void Start(Object *self) {
        old_Start(self);
        instance = self;
        LOGI("EventPickUpItemManager instance: %p", self);
    }

    void (*old_OnDisable)(...);

    void OnDisable(Object *self) {
        old_OnDisable(self);
        instance = nullptr;
        LOGI("EventPickUpItemManager instance: nullptr");
    }

    void init() {
        Tools::Hook(get_class()->find_method("InitializeEventPickUpManager")->methodPointer, (void *) Start, (void **) &old_Start);
        Tools::Hook(get_class()->find_method("OnDisable")->methodPointer, (void *) OnDisable, (void **) &old_OnDisable);
    }
}
