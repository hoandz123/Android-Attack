#include "PlayLog.h"
#include "JsonConvert.h"
#include <Includes/Logger.h>
#include <API/Il2CppApi.h>
#include <string>

namespace JsonConvert {
    Class *get_class() {
        return FindClass("Newtonsoft.Json.JsonConvert");
    }
    String *SerializeObject(Object *obj) {
        Il2CppObject *raw = get_class()->find_method("SerializeObject", 1)->static_invoke<String *>(obj);
        if (!raw) {
            LOGE("JsonConvert::SerializeObject returned null");
            return String::Create("");
        }
        return (String *) raw;
    }
}

