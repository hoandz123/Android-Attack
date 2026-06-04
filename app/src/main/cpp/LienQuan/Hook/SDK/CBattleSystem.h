#pragma once

#include <API/Il2CppApi.h>
#include <API/Vector2.h>
#include <API/Vector3.h>

namespace lienquan {
namespace CBattleSystem {

Object *GetInstance();
Object *GetTheMinimapSys();
Vector2 CvtWorld2UISM(const Vector3 &world);
Vector2 CvtWorld2UIBM(const Vector3 &world);

bool ReadLinkerMeta(Object *linker, uint32_t &configId, uint32_t &skinId);

// CHeroInfo.GetHeroName(heroId) — dump 0x763b834; heroId = ActorMeta.ConfigId
std::string GetHeroName(uint32_t configId);

Object *GetActorLinkerByObjId(unsigned int objId);

}
}
