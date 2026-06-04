#pragma once

#include <API/Il2CppApi.h>

namespace lienquan {

namespace LLogicCore {

Object *GetInstance();
Object *GetCurUpdatingDesk();
Object *GetInstances();
int DeskCount();
Object *DeskAt(int index);

} // namespace LLogicCore
} // namespace lienquan
