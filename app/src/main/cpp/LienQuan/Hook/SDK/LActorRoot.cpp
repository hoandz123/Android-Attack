#include "LActorRoot.h"
#include "LGameActorMgr.h"
#include <cstring>
#include <Includes/obfuscate.h>

namespace lienquan {

namespace {

Class *Vint3Class() {
    static Class *cached = nullptr;
    if (!cached) cached = FindClass(OBF("VInt3"));
    return cached;
}

Vector3 VInt3ToVector3(const VInt3 &v) {
    Class *cls = Vint3Class();
    if (!cls) return {};

    VInt3 copy = v;
    Il2CppMethod *getVec3 = cls->find_method(OBF("get_vec3"), 0);
    if (getVec3) return call_method<Vector3>(getVec3, &copy);

    Il2CppMethod *opExplicit = cls->find_method(OBF("op_Explicit"), 1);
    if (opExplicit) return call_method<Vector3>(opExplicit, nullptr, copy);

    return {};
}

struct CrypticInt32Val {
    uint8_t head[8];
    int32_t decrypt;
    int32_t cryptic;
};

bool IsHeroConfigId(uint32_t id) { return id >= 100u; }

int DecodeEnCId(const char *base, size_t encOff) {
    if (!base || encOff == static_cast<size_t>(-1)) return 0;
    static Class *cls = nullptr;
    static Il2CppMethod *opImplicit = nullptr;
    if (!cls) cls = FindClass(OBF("CrypticInt32"));
    if (!cls) return 0;
    if (!opImplicit) opImplicit = cls->find_method(OBF("op_Implicit"), 1);
    if (!opImplicit) return 0;
    CrypticInt32Val enc{};
    std::memcpy(&enc, base + encOff, sizeof(enc));
    try {
        return call_method<int>(opImplicit, nullptr, enc);
    } catch (...) {
        return 0;
    }
}

bool ReadMetaStruct(const char *base, uint32_t &configId, uint32_t &skinId) {
    configId = skinId = 0;
    if (!base) return false;
    const size_t cfgOff = GET_FIELD("Project.Plugins_d.dll", "", "ActorMeta", "ConfigId");
    const size_t encOff = GET_FIELD("Project.Plugins_d.dll", "", "ActorMeta", "EnCId");
    const size_t skinOff = GET_FIELD("Project.Plugins_d.dll", "", "ActorMeta", "SkinID");
    if (cfgOff == static_cast<size_t>(-1)) return false;

    int cfg = 0;
    std::memcpy(&cfg, base + cfgOff, sizeof(cfg));
    if (cfg == 0) cfg = DecodeEnCId(base, encOff);
    if (skinOff != static_cast<size_t>(-1))
        std::memcpy(&skinId, base + skinOff, sizeof(skinId));
    configId = static_cast<uint32_t>(cfg);
    return IsHeroConfigId(configId);
}

} // namespace

namespace LActorRoot {

Object *FromPoolHandle(Object *poolHandle) {
    if (!poolHandle) return nullptr;
    return poolHandle->get_field_object<Object *>(OBF("_handleObj"));
}

unsigned int GetObjID(Object *root) {
    if (!root) return 0;
    return static_cast<unsigned int>(root->invoke_method<unsigned int>(OBF("get_ObjID")));
}

unsigned int GetActorPlayerId(Object *root) {
    if (!root) return 0;
    const size_t metaOff = GET_FIELD("Project.Plugins_d.dll", "NucleusDrive.Logic", "LActorRoot", "TheActorMeta");
    const size_t playerOff = GET_FIELD("Project.Plugins_d.dll", "", "ActorMeta", "PlayerId");
    if (metaOff == static_cast<size_t>(-1) || playerOff == static_cast<size_t>(-1)) return 0;
    unsigned int playerId = 0;
    std::memcpy(&playerId, reinterpret_cast<const char *>(root) + metaOff + playerOff, sizeof(playerId));
    return playerId;
}

int GetEnemyCamp(Object *root) {
    if (!root) return -1;
    Class *cls = root->get_class();
    if (!cls) return -1;
    Il2CppMethod *m = cls->find_method(OBF("GiveMyEnemyCamp"), 0);
    if (!m) return -1;
    return call_method<int>(m, (void *)root);
}

Vector3 GetWorldPosition(Object *root) {
    if (!root) return {};
    Class *cls = root->get_class();
    if (cls && cls->find_method(OBF("get_location"), 0)) {
        VInt3 copy = root->invoke_method<VInt3>(OBF("get_location"));
        return VInt3ToVector3(copy);
    }
    VInt3 loc{};
    const size_t locOff = GET_FIELD("Project.Plugins_d.dll", "NucleusDrive.Logic", "LActorRoot", "_location");
    if (locOff != static_cast<size_t>(-1))
        std::memcpy(&loc, reinterpret_cast<const char *>(root) + locOff, sizeof(loc));
    return VInt3ToVector3(loc);
}

int HeroCount(Object *gameActorMgr) { return LGameActorMgr::HeroCount(gameActorMgr); }

Object *HeroAt(int index, Object *gameActorMgr) {
    return FromPoolHandle(LGameActorMgr::HeroHandleAt(index, gameActorMgr));
}

bool ReadActorConfigMeta(Object *root, uint32_t &configId, uint32_t &skinId) {
    configId = skinId = 0;
    const size_t actorCfgFieldOff =
        GET_FIELD("Project.Plugins_d.dll", "NucleusDrive.Logic", "LActorRoot", "actorConfig");
    Object *actorConfig = nullptr;
    if (actorCfgFieldOff != static_cast<size_t>(-1))
        std::memcpy(&actorConfig, reinterpret_cast<const char *>(root) + actorCfgFieldOff,
                    sizeof(actorConfig));
    if (!actorConfig || !actorConfig->get_class()) return false;
    const size_t cfgOff =
        GET_FIELD("Project.Plugins_d.dll", "NucleusDrive.Logic", "ActorConfigData", "ConfigID");
    const size_t skinOff =
        GET_FIELD("Project.Plugins_d.dll", "NucleusDrive.Logic", "ActorConfigData", "SkinID");
    int cfg = 0;
    if (cfgOff != static_cast<size_t>(-1))
        std::memcpy(&cfg, reinterpret_cast<const char *>(actorConfig) + cfgOff, sizeof(cfg));
    if (skinOff != static_cast<size_t>(-1))
        std::memcpy(&skinId, reinterpret_cast<const char *>(actorConfig) + skinOff, sizeof(skinId));
    configId = static_cast<uint32_t>(cfg);
    return IsHeroConfigId(configId);
}

bool ReadActorMeta(Object *root, uint32_t &configId, uint32_t &skinId) {
    configId = skinId = 0;
    if (!root || !root->get_class()) return false;

    if (ReadActorConfigMeta(root, configId, skinId)) return true;

    const size_t metaOff =
        GET_FIELD("Project.Plugins_d.dll", "NucleusDrive.Logic", "LActorRoot", "TheActorMeta");
    if (metaOff != static_cast<size_t>(-1)) {
        const char *base = reinterpret_cast<const char *>(root) + metaOff;
        if (ReadMetaStruct(base, configId, skinId)) return true;
    }

    const size_t staticOff =
        GET_FIELD("Project.Plugins_d.dll", "NucleusDrive.Logic", "LActorRoot", "TheStaticData");
    const size_t staticMetaOff =
        GET_FIELD("Project.Plugins_d.dll", "", "ActorStaticData", "TheActorMeta");
    if (staticOff != static_cast<size_t>(-1) && staticMetaOff != static_cast<size_t>(-1)) {
        const char *base =
            reinterpret_cast<const char *>(root) + staticOff + staticMetaOff;
        if (ReadMetaStruct(base, configId, skinId)) return true;
    }
    return false;
}

} // namespace LActorRoot
} // namespace lienquan
