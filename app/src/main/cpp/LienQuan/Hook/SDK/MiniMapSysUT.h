#pragma once

#include <API/Vector3.h>

namespace lienquan {
namespace MiniMapSysUT {

bool WorldToScreenPoint(const Vector3 &world, bool bMiniMap, float screenH, float &outX, float &outY);

}
}
