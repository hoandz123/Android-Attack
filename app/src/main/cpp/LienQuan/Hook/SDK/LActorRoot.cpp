#include "LActorRoot.h"
#include "LGameActorMgr.h"
#include <cstring>
#include <string>
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

bool IsPlausibleConfigId(uint32_t id) { return id > 0 && id < 10000000u; }

bool PtrLooksValid(Object *o) {
    if (!o) return false;
    const auto u = reinterpret_cast<uintptr_t>(o);
    if ((u & 3) != 0 || u <= 0x10000) return false;
    return true;
}

bool ManagedObjOk(Object *o) {
    if (!PtrLooksValid(o)) return false;
    try {
        return o->get_class() != nullptr;
    } catch (...) {
        return false;
    }
}

int DecodeCurSkillCdMs(const char *base, size_t fieldOff) {
    if (!base || fieldOff == static_cast<size_t>(-1)) return 0;
    int32_t lo = 0;
    int32_t hi = 0;
    std::memcpy(&lo, base + fieldOff, sizeof(lo));
    std::memcpy(&hi, base + fieldOff + sizeof(lo), sizeof(hi));
    return lo ^ hi;
}

int SkillSlotTypeValue(const char *slotName) {
    static Class *cls = nullptr;
    if (!cls) cls = FindClass(OBF("SkillSlotType"));
    if (!cls) return -1;
    return cls->get_enum_value<int>(slotName);
}

Object *SkillControlOf(Object *root) {
    if (!ManagedObjOk(root)) return nullptr;
    Class *cls = root->get_class();
    if (!cls || !cls->get_field_from_name(OBF("SkillControl"))) return nullptr;
    try {
        return root->get_field_object<Object *>(OBF("SkillControl"));
    } catch (...) {
        return nullptr;
    }
}

bool SuspiciousSkillCd(int rawCdMs, int cdSec, bool unlock, bool ready) {
    if (rawCdMs > 300000 || rawCdMs < 0) return true;
    if (cdSec > 300) return true;
    if (unlock && ready && rawCdMs > 0) return true;
    if (unlock && !ready && rawCdMs <= 0) return true;
    return false;
}

bool ReadLogicSkillSlot(Object *skillControl, int slotType, LActorRoot::SkillCooldownSlot &slot) {
    if (!ManagedObjOk(skillControl) || slotType < 0) return false;
    Class *ctrlCls = skillControl->get_class();
    if (!ctrlCls || !ctrlCls->find_method(OBF("GetSkillSlot"), 1)) return false;
    Object *skillSlot = nullptr;
    try {
        skillSlot = skillControl->invoke_method<Object *>(OBF("GetSkillSlot"), slotType);
    } catch (...) {
        return false;
    }
    if (!ManagedObjOk(skillSlot)) return false;
    Class *slotCls = skillSlot->get_class();
    if (!slotCls || !slotCls->find_method(OBF("IsUnLock"), 0) || !slotCls->find_method(OBF("get_IsCDReady"), 0)) return false;
    bool unlock = false;
    bool ready = false;
    int cdMs = 0;
    try {
        unlock = skillSlot->invoke_method<bool>(OBF("IsUnLock"));
        ready = skillSlot->invoke_method<bool>(OBF("get_IsCDReady"));
        const size_t cdOff = GET_FIELD("Project.Plugins_d.dll", "NucleusDrive.Logic", "SkillSlot", "<CurSkillCD>k__BackingField");
        if (cdOff != static_cast<size_t>(-1)) {
            const char *slotBase = reinterpret_cast<const char *>(skillSlot);
            cdMs = DecodeCurSkillCdMs(slotBase, cdOff);
        }
    } catch (...) {
        return false;
    }
    slot.valid = true;
    slot.unlocked = unlock;
    slot.ready = unlock && (ready || cdMs <= 0);
    int cdSec = unlock && !ready && cdMs > 0 ? (cdMs + 999) / 1000 : 0;
    if (SuspiciousSkillCd(cdMs, cdSec, unlock, ready)) cdSec = 0;
    slot.cdSec = cdSec;
    return true;
}

Object *ValueComponentOf(Object *root) {
    if (!root) return nullptr;
    return root->get_field_object<Object *>(OBF("ValueComponent"));
}

Object *ActorControlOf(Object *root) {
    if (!root) return nullptr;
    return root->get_field_object<Object *>(OBF("ActorControl"));
}

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
    return IsPlausibleConfigId(configId);
}

} // namespace

namespace LActorRoot {

Object *FromPoolHandle(Object *poolHandle) {
    if (!ManagedObjOk(poolHandle)) return nullptr;
    try {
        Class *poolCls = poolHandle->get_class();
        if (!poolCls || !poolCls->get_field_from_name(OBF("_handleObj"))) return nullptr;
        Object *root = poolHandle->get_field_object<Object *>(OBF("_handleObj"));
        if (!ManagedObjOk(root)) return nullptr;
        Class *rootCls = root->get_class();
        if (!rootCls || rootCls->get_name() != OBF("LActorRoot")) return nullptr;
        return root;
    } catch (...) {
        return nullptr;
    }
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

Vector3 VInt3ToWorldM(const VInt3 &v) {
    constexpr float k = 1.f / 1000.f;
    return {static_cast<float>(v.x) * k, static_cast<float>(v.y) * k, static_cast<float>(v.z) * k};
}

bool TryReadLocationField(Object *root, VInt3 &out) {
    if (!ManagedObjOk(root)) return false;
    const size_t locOff = GET_FIELD("Project.Plugins_d.dll", "NucleusDrive.Logic", "LActorRoot", "_location");
    if (locOff == static_cast<size_t>(-1)) return false;
    std::memcpy(&out, reinterpret_cast<const char *>(root) + locOff, sizeof(out));
    return true;
}

Vector3 GetWorldPosition(Object *root) {
    if (!ManagedObjOk(root)) return {};
    VInt3 loc{};
    if (TryReadLocationField(root, loc)) {
        const Vector3 pos = VInt3ToWorldM(loc);
        if (pos.x != 0.f || pos.y != 0.f || pos.z != 0.f) return pos;
    }
    try {
        Class *cls = root->get_class();
        if (cls && cls->find_method(OBF("get_location"), 0)) {
            VInt3 copy = root->invoke_method<VInt3>(OBF("get_location"));
            return VInt3ToVector3(copy);
        }
        return VInt3ToWorldM(loc);
    } catch (...) {
        return VInt3ToWorldM(loc);
    }
}

int HeroCount(Object *gameActorMgr) { return LGameActorMgr::HeroCount(gameActorMgr); }

Object *HeroAt(int index, Object *gameActorMgr) {
    return FromPoolHandle(LGameActorMgr::HeroHandleAt(index, gameActorMgr));
}

bool ReadActorConfigMeta(Object *root, uint32_t &configId, uint32_t &skinId) {
    configId = skinId = 0;
    const size_t actorCfgFieldOff = GET_FIELD("Project.Plugins_d.dll", "NucleusDrive.Logic", "LActorRoot", "actorConfig");
    Object *actorConfig = nullptr;
    if (actorCfgFieldOff != static_cast<size_t>(-1)) std::memcpy(&actorConfig, reinterpret_cast<const char *>(root) + actorCfgFieldOff, sizeof(actorConfig));
    if (!actorConfig || !actorConfig->get_class()) return false;
    const size_t cfgOff = GET_FIELD("Project.Plugins_d.dll", "NucleusDrive.Logic", "ActorConfigData", "ConfigID");
    const size_t skinOff = GET_FIELD("Project.Plugins_d.dll", "NucleusDrive.Logic", "ActorConfigData", "SkinID");
    int cfg = 0;
    if (cfgOff != static_cast<size_t>(-1))
        std::memcpy(&cfg, reinterpret_cast<const char *>(actorConfig) + cfgOff, sizeof(cfg));
    if (skinOff != static_cast<size_t>(-1))
        std::memcpy(&skinId, reinterpret_cast<const char *>(actorConfig) + skinOff, sizeof(skinId));
    configId = static_cast<uint32_t>(cfg);
    return IsPlausibleConfigId(configId);
}

bool ReadActorMeta(Object *root, uint32_t &configId, uint32_t &skinId) {
    configId = skinId = 0;
    if (!root || !root->get_class()) return false;

    uint32_t cfg = 0;
    uint32_t skin = 0;
    if (ReadActorConfigMeta(root, cfg, skin) && IsPlausibleConfigId(cfg)) {
        configId = cfg;
        skinId = skin;
        return true;
    }

    const size_t metaOff = GET_FIELD("Project.Plugins_d.dll", "NucleusDrive.Logic", "LActorRoot", "TheActorMeta");
    if (metaOff != static_cast<size_t>(-1)) {
        const char *base = reinterpret_cast<const char *>(root) + metaOff;
        cfg = skin = 0;
        if (ReadMetaStruct(base, cfg, skin) && IsPlausibleConfigId(cfg)) {
            configId = cfg;
            skinId = skin;
            return true;
        }
    }

    const size_t staticOff = GET_FIELD("Project.Plugins_d.dll", "NucleusDrive.Logic", "LActorRoot", "TheStaticData");
    const size_t staticMetaOff = GET_FIELD("Project.Plugins_d.dll", "", "ActorStaticData", "TheActorMeta");
    if (staticOff != static_cast<size_t>(-1) && staticMetaOff != static_cast<size_t>(-1)) {
        const char *base = reinterpret_cast<const char *>(root) + staticOff + staticMetaOff;
        cfg = skin = 0;
        if (ReadMetaStruct(base, cfg, skin) && IsPlausibleConfigId(cfg)) {
            configId = cfg;
            skinId = skin;
            return true;
        }
    }
    return false;
}

int GetActorCamp(Object *root) {
    if (!root) return -1;
    const size_t metaOff = GET_FIELD("Project.Plugins_d.dll", "NucleusDrive.Logic", "LActorRoot", "TheActorMeta");
    const size_t campOff = GET_FIELD("Project.Plugins_d.dll", "", "ActorMeta", "ActorCamp");
    if (metaOff == static_cast<size_t>(-1) || campOff == static_cast<size_t>(-1)) return -1;
    int camp = -1;
    std::memcpy(&camp, reinterpret_cast<const char *>(root) + metaOff + campOff, sizeof(camp));
    return camp;
}


bool ReadActorHp(Object *root, int &hp, int &maxHp) {
    hp = maxHp = 0;
    Object *valueComp = ValueComponentOf(root);
    if (!valueComp || !valueComp->get_class()) return false;
    Class *cls = valueComp->get_class();
    if (!cls->find_method(OBF("get_actorHp"), 0) || !cls->find_method(OBF("get_actorHpTotal"), 0)) return false;
    try {
        hp = valueComp->invoke_method<int>(OBF("get_actorHp"));
        maxHp = valueComp->invoke_method<int>(OBF("get_actorHpTotal"));
        return true;
    } catch (...) {
        return false;
    }
}

bool ReadActorSoulLevel(Object *root, int &level) {
    level = 0;
    Object *valueComp = ValueComponentOf(root);
    if (!valueComp || !valueComp->get_class()) return false;
    if (!valueComp->get_class()->find_method(OBF("get_actorSoulLevel"), 0)) return false;
    try {
        level = valueComp->invoke_method<int>(OBF("get_actorSoulLevel"));
        return true;
    } catch (...) {
        return false;
    }
}

bool IsActorDead(Object *root) {
    Object *actorControl = ActorControlOf(root);
    if (!actorControl || !actorControl->get_class()) return false;
    if (!actorControl->get_class()->find_method(OBF("get_IsDeadState"), 0)) return false;
    try {
        return actorControl->invoke_method<bool>(OBF("get_IsDeadState"));
    } catch (...) {
        return false;
    }
}

bool ReadActorInfo(Object *root, ActorInfo &out) {
    out = {};
    if (!root || !root->get_class()) return false;
    out.objId = GetObjID(root);
    out.playerId = GetActorPlayerId(root);
    ReadActorMeta(root, out.configId, out.skinId);
    const int camp = GetActorCamp(root);
    if (camp >= 0) {
        out.actorCamp = camp;
        out.hasCamp = true;
    }
    out.enemyCamp = GetEnemyCamp(root);
    out.worldPos = GetWorldPosition(root);
    out.hasHp = ReadActorHp(root, out.hp, out.maxHp);
    out.hasSoulLevel = ReadActorSoulLevel(root, out.soulLevel);
    Object *actorControl = ActorControlOf(root);
    if (actorControl && actorControl->get_class() && actorControl->get_class()->find_method(OBF("get_IsDeadState"), 0)) {
        out.hasDeadState = true;
        out.isDead = IsActorDead(root);
    }
    String *nameStr = root->get_field_object<String *>(OBF("name"));
    if (nameStr && nameStr->get_length() > 0) out.actorName = nameStr->to_string();
    return out.objId != 0;
}

bool ReadSkillCooldowns(Object *root, SkillCooldownInfo &out) {
    out = {};
    if (!ManagedObjOk(root)) return false;
    try {
        Object *skillControl = SkillControlOf(root);
        if (!ManagedObjOk(skillControl)) return false;
        static const char *kSlotNames[kSkillCooldownSlots] = {
            OBF("SLOT_SKILL_1"),
            OBF("SLOT_SKILL_2"),
            OBF("SLOT_SKILL_3"),
            OBF("SLOT_SKILL_5"),
        };
        int validN = 0;
        for (int i = 0; i < kSkillCooldownSlots; ++i) {
            const int slotType = SkillSlotTypeValue(kSlotNames[i]);
            if (slotType < 0) continue;
            if (ReadLogicSkillSlot(skillControl, slotType, out.slots[i])) validN++;
        }
        out.hasData = validN > 0;
        return out.hasData;
    } catch (...) {
        return false;
    }
}

int SkillSlotTypeOf(const char *slotName) { return SkillSlotTypeValue(slotName); }

bool ReadSkillSlot(Object *root, int slotType, SkillCooldownSlot &out) {
    out = {};
    if (!ManagedObjOk(root) || slotType < 0) return false;
    Object *skillControl = SkillControlOf(root);
    if (!ManagedObjOk(skillControl)) return false;
    return ReadLogicSkillSlot(skillControl, slotType, out);
}

} // namespace LActorRoot
} // namespace lienquan
