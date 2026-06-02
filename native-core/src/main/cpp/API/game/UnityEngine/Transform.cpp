//
// Created by HMGTEAM on 11/05/2025.
//

#include "Transform.h"
#include "API/Il2CppApi.h"
using namespace UnityEngine;

Vector3 Transform::get_position() {
    return ((Il2CppObject*)this)->invoke_method<Vector3>("get_position");
}
void Transform::set_position(Vector3 position) {
    ((Il2CppObject*)this)->invoke_method<void>("set_position", position);
}
Vector3 Transform::get_localPosition() {
    return ((Il2CppObject*)this)->invoke_method<Vector3>("get_localPosition");
}
void Transform::set_localPosition(Vector3 position) {
    ((Il2CppObject*)this)->invoke_method<void>("set_localPosition", position);
}
Quaternion Transform::get_rotation() {
    return ((Il2CppObject*)this)->invoke_method<Quaternion>("get_rotation");
}
void Transform::set_rotation(Quaternion rotation) {
    ((Il2CppObject*)this)->invoke_method<void>("set_rotation", rotation);
}

Vector3 Transform::get_forward() {
    return ((Il2CppObject*)this)->invoke_method<Vector3>("get_forward");
}
void Transform::set_forward(Vector3 forward) {
    ((Il2CppObject*)this)->invoke_method<void>("set_forward", forward);
}
Vector3 Transform::get_right() {
    return ((Il2CppObject*)this)->invoke_method<Vector3>("get_right");
}
void Transform::set_right(Vector3 right) {
    ((Il2CppObject*)this)->invoke_method<void>("set_right", right);
}
Vector3 Transform::get_up() {
    return ((Il2CppObject*)this)->invoke_method<Vector3>("get_up");
}
void Transform::set_up(Vector3 up) {
    ((Il2CppObject*)this)->invoke_method<void>("set_up", up);
}
Vector3 Transform::get_localScale() {
    return ((Il2CppObject*)this)->invoke_method<Vector3>("get_localScale");
}
void Transform::set_localScale(Vector3 scale) {
    ((Il2CppObject*)this)->invoke_method<void>("set_localScale", scale);
}
