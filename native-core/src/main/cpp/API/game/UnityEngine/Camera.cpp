//
// Created by HMGTEAM on 11/05/2025.
//

#include "Camera.h"
#include "API/Il2CppApi.h"
#include "UnityEngine/Transform.h"
#include "UnityEngine/Object.h"

using namespace UnityEngine;

Camera *Camera::get_main() {
    static auto _ = (Camera* (*)()) GET_METHOD("Camera", "get_main", 0);
    return _();
}
Camera *Camera::get_current() {
    static auto _ = (Camera* (*)()) GET_METHOD("Camera", "get_current", 0);
    return _();
}
Vector3 Camera::WorldToScreenPoint(Vector3 position) {
    static auto _ = (Vector3 (*)(Camera*, Vector3)) GET_METHOD("Camera", "WorldToScreenPoint", 1);
    return _(this, position);
}