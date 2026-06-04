#include "SkinUnlock.h"
#include "../Config/Config.h"
#include <API/Il2CppApi.h>
#include <Tools/Tools.h>
#include <Includes/obfuscate.h>
#include <string>

namespace lienquan {
namespace SkinUnlock {

namespace {

struct TdrReadBuf {};
enum TdrErrorType : int32_t { TDR_NO_ERROR = 0 };

size_t offHero = 0, offSkin = 0;
uint32_t pickHero = 0;
uint16_t pickSkin = 0;
bool unpackOn = false;

TdrErrorType (*old_unpack)(void *, TdrReadBuf &, uint32_t) = nullptr;
void (*old_RefreshHeroPanel)(void *, bool, bool, bool) = nullptr;
void (*old_OnClickSelectHeroSkin)(void *, uint32_t, uint32_t) = nullptr;
bool (*old_IsCanUseSkin)(void *, uint32_t, uint32_t) = nullptr;
uint32_t (*old_GetHeroWearSkinId)(void *, uint32_t) = nullptr;
bool (*old_IsHaveHeroSkin)(void *, uint32_t, uint32_t, bool) = nullptr;
bool (*old_PersonalIsOpen)() = nullptr;
int (*old_PersonalBtnId)() = nullptr;

void patchUnpack(void *inst) {
    if (!unpackOn || !inst || !offHero || !offSkin) return;
    auto *p = (uint8_t *)inst;
    if (*(uint32_t *)(p + offHero) != pickHero || !pickHero || !pickSkin) return;
    *(uint16_t *)(p + offSkin) = pickSkin;
}

TdrErrorType hook_unpack(void *inst, TdrReadBuf &buf, uint32_t ver) {
    TdrErrorType r = old_unpack ? old_unpack(inst, buf, ver) : TDR_NO_ERROR;
    if (gLQConfig.main.unlockSkin) patchUnpack(inst);
    return r;
}

void hook_OnClickSelectHeroSkin(void *self, uint32_t heroId, uint32_t skinId) {
    if (gLQConfig.main.unlockSkin && heroId && old_RefreshHeroPanel)
        old_RefreshHeroPanel(self, true, true, true);
    if (old_OnClickSelectHeroSkin) old_OnClickSelectHeroSkin(self, heroId, skinId);
}

bool hook_IsCanUseSkin(void *self, uint32_t heroId, uint32_t skinId) {
    if (gLQConfig.main.unlockSkin) {
        if (heroId) {
            pickHero = heroId;
            pickSkin = (uint16_t)skinId;
        }
        return true;
    }
    return old_IsCanUseSkin ? old_IsCanUseSkin(self, heroId, skinId) : false;
}

uint32_t hook_GetHeroWearSkinId(void *self, uint32_t heroId) {
    if (gLQConfig.main.unlockSkin) {
        unpackOn = true;
        return pickSkin;
    }
    return old_GetHeroWearSkinId ? old_GetHeroWearSkinId(self, heroId) : 0;
}

bool hook_IsHaveHeroSkin(void *self, uint32_t heroId, uint32_t skinId, bool limited) {
    if (gLQConfig.main.unlockSkin) return true;
    return old_IsHaveHeroSkin ? old_IsHaveHeroSkin(self, heroId, skinId, limited) : false;
}

bool hook_PersonalIsOpen() {
    if (gLQConfig.main.unlockButton) return true;
    return old_PersonalIsOpen ? old_PersonalIsOpen() : false;
}

int concatId(int h, int s) {
    int v = 0;
    for (char c : std::to_string(h)) v = v * 10 + (c - '0');
    for (char c : std::to_string(s)) v = v * 10 + (c - '0');
    return v;
}

int hook_PersonalBtnId() {
    const int h = gLQConfig.main.buttonHeroId, s = gLQConfig.main.buttonSkinId;
    if (gLQConfig.main.unlockButton && h && s) return concatId(h, s);
    return old_PersonalBtnId ? old_PersonalBtnId() : 0;
}

} // namespace

void init() {
    offHero = GET_FIELD("AovTdr.dll", "CSProtocol", "COMDT_HERO_COMMON_INFO", "dwHeroID");
    offSkin = GET_FIELD("AovTdr.dll", "CSProtocol", "COMDT_HERO_COMMON_INFO", "wSkinID");
    if (!offHero) offHero = 0x8;
    if (!offSkin) offSkin = 0x3a;

    old_RefreshHeroPanel = (decltype(old_RefreshHeroPanel))GET_METHOD("Project_d.dll", "Assets.Scripts.GameSystem", "HeroSelectNormalWindow", "RefreshHeroPanel", 3);

    Tools::Hook((void *)GET_METHOD("AovTdr.dll", "CSProtocol", "COMDT_HERO_COMMON_INFO", "unpack", 2), (void *)hook_unpack, (void **)&old_unpack);
    Tools::Hook((void *)GET_METHOD("Project_d.dll", "Assets.Scripts.GameSystem", "HeroSelectNormalWindow", "OnClickSelectHeroSkin", 2), (void *)hook_OnClickSelectHeroSkin, (void **)&old_OnClickSelectHeroSkin);
    Tools::Hook((void *)GET_METHOD("Project_d.dll", "Assets.Scripts.GameSystem", "CRoleInfo", "IsCanUseSkin", 2), (void *)hook_IsCanUseSkin, (void **)&old_IsCanUseSkin);
    Tools::Hook((void *)GET_METHOD("Project_d.dll", "Assets.Scripts.GameSystem", "CRoleInfo", "GetHeroWearSkinId", 1), (void *)hook_GetHeroWearSkinId, (void **)&old_GetHeroWearSkinId);
    Tools::Hook((void *)GET_METHOD("Project_d.dll", "Assets.Scripts.GameSystem", "CRoleInfo", "IsHaveHeroSkin", 3), (void *)hook_IsHaveHeroSkin, (void **)&old_IsHaveHeroSkin);
    Tools::Hook((void *)GET_METHOD("Project_d.dll", "Assets.Scripts.GameSystem", "PersonalButton", "IsOpen", 0), (void *)hook_PersonalIsOpen, (void **)&old_PersonalIsOpen);
    Tools::Hook((void *)GET_METHOD("Project_d.dll", "Assets.Scripts.GameSystem", "PersonalButton", "get_PersonalBtnId", 0), (void *)hook_PersonalBtnId, (void **)&old_PersonalBtnId);
}

} // namespace SkinUnlock
} // namespace lienquan
