#pragma once

#include <API/Il2CppApi.h>
#include <cstdint>

namespace lienquan {
namespace ActorManager {

Object *GetInstance();
Object *HeroList(Object *mgr = nullptr);
int HeroCount(Object *mgr = nullptr);
Object *HeroHandleAt(int index, Object *mgr = nullptr);
Object *HeroLinkerAt(int index, Object *mgr = nullptr);
Object *LinkerFromPoolHandle(Object *poolHandle);

} // namespace ActorManager
} // namespace lienquan
