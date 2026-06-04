#pragma once

#include <API/Il2CppApi.h>
#include <API/Vector2.h>

namespace lienquan {
namespace MinimapSys {

enum class EMapType : int { None = 0, Mini = 1, Big = 2, Skill = 3 };

Object *Get();
EMapType CurMapType(Object *minimap = nullptr);
bool IsMiniMapMode(EMapType type);
Vector2 GetFinalScreenPos(Object *minimap, bool bMiniMap);
Vector2 GetFinalScreenSize(Object *minimap, bool bMiniMap);

}
}
