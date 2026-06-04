#include "MiniMapSysUT.h"
#include <API/Il2CppApi.h>
#include <Includes/obfuscate.h>

namespace lienquan {
namespace MiniMapSysUT {

namespace {

Class *GetClass() {
    static Class *cached = nullptr;
    if (!cached) cached = FindClass(OBF("Assets.Scripts.GameSystem.MiniMapSysUT"));
    return cached;
}

} // namespace

bool WorldToScreenPoint(const Vector3 &world, bool bMiniMap, float screenH, float &outX, float &outY) {
    Class *cls = GetClass();
    if (!cls) return false;
    Il2CppMethod *m = cls->find_method(OBF("Set3DUIWorldPos_ByScreenPoint"), 4);
    if (!m) return false;
    float fx = 0.f, fy = 0.f;
    (void)call_method<Vector3>(m, (void *)nullptr, world, bMiniMap, &fx, &fy);
    outX = fx;
    outY = screenH - fy;
    return true;
}

}
}
