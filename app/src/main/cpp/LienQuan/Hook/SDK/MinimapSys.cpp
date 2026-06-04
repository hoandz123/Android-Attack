#include "MinimapSys.h"
#include "CBattleSystem.h"
#include <Includes/obfuscate.h>

namespace lienquan {
namespace MinimapSys {

Object *Get() { return CBattleSystem::GetTheMinimapSys(); }

EMapType CurMapType(Object *minimap) {
    if (!minimap) minimap = Get();
    if (!minimap) return EMapType::None;
    return static_cast<EMapType>(minimap->invoke_method<int>(OBF("CurMapType")));
}

bool IsMiniMapMode(EMapType type) { return type != EMapType::None && type != EMapType::Big; }

Vector2 GetFinalScreenPos(Object *minimap, bool bMiniMap) {
    if (!minimap) minimap = Get();
    if (!minimap) return {};
    return bMiniMap ? minimap->invoke_method<Vector2>(OBF("GetMMFianlScreenPos")) : minimap->invoke_method<Vector2>(OBF("GetBMFianlScreenPos"));
}

Vector2 GetFinalScreenSize(Object *minimap, bool bMiniMap) {
    if (!minimap) minimap = Get();
    if (!minimap) return {};
    return bMiniMap ? minimap->invoke_method<Vector2>(OBF("get_mmFinalScreenSize")) : minimap->invoke_method<Vector2>(OBF("get_bmFinalScreenSize"));
}

}
}
