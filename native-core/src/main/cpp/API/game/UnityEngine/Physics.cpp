//
// Created by HMGTEAM on 11/06/2025.
//

#include "Physics.h"
#include "API/Il2CppApi.h"

bool Physics::Raycast(Vector3 origin, Vector3 direction, void* hitInfo) {
    static auto _ = (bool (*)(...)) GET_METHOD("UnityEngine.PhysicsModule.dll", "UnityEngine", "Physics", "Raycast", 3, 2);
    return _(origin, direction, hitInfo);
}
