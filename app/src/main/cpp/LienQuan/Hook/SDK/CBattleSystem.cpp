#include "CBattleSystem.h"
#include "LActorRoot.h"
#include <API/Il2CppApi.h>
#include <cstring>
#include <Includes/obfuscate.h>

#define LOGGER_TAG "ATTACK_CBattle"
#include <Includes/Logger.h>

namespace lienquan {
namespace CBattleSystem {

namespace {

Class *GetClass() {
    static Class *cached = nullptr;
    if (!cached) cached = FindClass(OBF("Assets.Scripts.GameSystem.CBattleSystem"));
    return cached;
}

Class *CHeroInfoClass() {
    static Class *cached = nullptr;
    if (!cached) cached = FindClass(OBF("Assets.Scripts.GameSystem.CHeroInfo"));
    return cached;
}

std::string PathFromManaged(String *s) {
    if (!s || s->get_length() <= 0) return {};
    return s->to_string();
}

bool IsActorLinker(Object *obj) {
    if (!obj || !obj->get_class()) return false;
    return obj->get_class()->get_name() == OBF("ActorLinker");
}

bool IsHeroConfigId(uint32_t id) { return id >= 100u; }

} // namespace

Object *GetInstance() {
    Class *cls = GetClass();
    if (!cls) return nullptr;
    Il2CppMethod *m = cls->find_method(OBF("GetInstance"), 0);
    if (!m) m = cls->find_method(OBF("get_instance"), 0);
    if (!m) return nullptr;
    return m->static_invoke<Object *>();
}

Object *GetTheMinimapSys() {
    Object *battle = GetInstance();
    if (!battle) return nullptr;
    return battle->invoke_method<Object *>(OBF("get_TheMinimapSys"));
}

Vector2 CvtWorld2UISM(const Vector3 &world) {
    Object *battle = GetInstance();
    if (!battle) return {};
    return battle->invoke_method<Vector2>(OBF("CvtWorld2UISM"), world);
}

Vector2 CvtWorld2UIBM(const Vector3 &world) {
    Object *battle = GetInstance();
    if (!battle) return {};
    return battle->invoke_method<Vector2>(OBF("CvtWorld2UIBM"), world);
}

bool ReadLinkerMeta(Object *linker, uint32_t &configId, uint32_t &skinId) {
    configId = skinId = 0;
    if (!IsActorLinker(linker)) return false;
    Il2CppMethod *getCfg = linker->get_class()->find_method(OBF("get_ConfigId"), 0);
    if (getCfg) {
        try {
            configId = linker->invoke_method<uint32_t>(OBF("get_ConfigId"));
        } catch (...) {
            configId = 0;
        }
    }
    const size_t metaOff = GET_FIELD("Project_d.dll", "Kyrios.Actor", "ActorLinker", "TheActorMeta");
    if (metaOff != static_cast<size_t>(-1)) {
        const char *base = reinterpret_cast<const char *>(linker) + metaOff;
        const size_t skinOff = GET_FIELD("Project.Plugins_d.dll", "", "ActorMeta", "SkinID");
        if (configId == 0) {
            const size_t cfgOff = GET_FIELD("Project.Plugins_d.dll", "", "ActorMeta", "ConfigId");
            if (cfgOff != static_cast<size_t>(-1)) {
                int cfg = 0;
                std::memcpy(&cfg, base + cfgOff, sizeof(cfg));
                configId = static_cast<uint32_t>(cfg);
            }
        }
        if (skinOff != static_cast<size_t>(-1))
            std::memcpy(&skinId, base + skinOff, sizeof(skinId));
    }
    return IsHeroConfigId(configId);
}

Object *GetActorLinkerByObjId(unsigned int objId) {
    if (!objId) return nullptr;
    static Il2CppMethod *getLinker = nullptr;
    if (!getLinker) {
        Class *cls = FindClass(OBF("Kyrios.Actor.VActorHelper"));
        if (cls) getLinker = cls->find_method(OBF("GetActorLinker"), 1);
    }
    if (!getLinker) return nullptr;
    try {
        Object *handle = getLinker->static_invoke<Object *>(objId);
        if (!handle || !handle->get_class()) return nullptr;
        Object *linker = LActorRoot::FromPoolHandle(handle);
        return IsActorLinker(linker) ? linker : nullptr;
    } catch (...) {
        LOGE(OBF("GetActorLinker threw objId=%u"), objId);
        return nullptr;
    }
}

std::string GetHeroName(uint32_t configId) {
    if (!IsHeroConfigId(configId)) return {};
    Class *cls = CHeroInfoClass();
    if (!cls) return {};
    static Il2CppMethod *m = nullptr;
    if (!m) m = cls->find_method(OBF("GetHeroName"), 1);
    if (!m) return {};
    try {
        return PathFromManaged(m->static_invoke<String *>(configId));
    } catch (...) {
        LOGE(OBF("CHeroInfo.GetHeroName threw cfg=%u"), configId);
        return {};
    }
}

} // namespace CBattleSystem
} // namespace lienquan
