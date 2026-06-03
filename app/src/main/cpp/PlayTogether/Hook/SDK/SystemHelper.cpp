#include "SystemHelper.h"

namespace SystemHelper {
    Class *get_class() {
        return FindClass(OBF("SystemHelper"));
    }
    Object *get_Dialog() {
        Class *cls = get_class();
        if (!cls) return nullptr;
        Il2CppMethod *method = cls->find_method(OBF("get_Dialog"), 0);
        if (!method) return nullptr;
        return method->static_invoke<Object *>();
    }
    Object *get_Layer() { return get_class()->find_method(OBF("get_Layer"), 0)->static_invoke<Object *>(); }
    Object *get_Table() { return get_class()->find_method(OBF("get_Table"), 0)->static_invoke<Object *>(); }
    Object *get_Map() { return get_class()->find_method(OBF("get_Map"), 0)->static_invoke<Object *>(); }
    Object *get_Actor() { return get_class()->find_method(OBF("get_Actor"), 0)->static_invoke<Object *>(); }
    Object *get_Cache() { return get_class()->find_method(OBF("get_Cache"), 0)->static_invoke<Object *>(); }
    Object *get_Hud() { return get_class()->find_method(OBF("get_Hud"), 0)->static_invoke<Object *>(); }
    Object *get_Mission() { return get_class()->find_method(OBF("get_Mission"), 0)->static_invoke<Object *>(); }
    Object *get_Fishing() {
        Class *cls = get_class();
        if (!cls) return nullptr;
        Il2CppMethod *method = cls->find_method(OBF("get_Fishing"), 0);
        if (!method) return nullptr;
        return method->static_invoke<Object *>();
    }
    Object *get_Ability() { return get_class()->find_method(OBF("get_Ability"), 0)->static_invoke<Object *>(); }
    Object *get_Escape() { return get_class()->find_method(OBF("get_Escape"), 0)->static_invoke<Object *>(); }
    Object *get_Equip() { return get_class()->find_method(OBF("get_Equip"), 0)->static_invoke<Object *>(); }
    Object *get_Insect() { return get_class()->find_method(OBF("get_Insect"), 0)->static_invoke<Object *>(); }
    Object *get_Collect() { return get_class()->find_method(OBF("get_Collect"), 0)->static_invoke<Object *>(); }
    Object *GetFrameWorkInstance() { return get_class()->find_method(OBF("GetFrameWorkInstance"), 0)->static_invoke<Object *>(); }
}
