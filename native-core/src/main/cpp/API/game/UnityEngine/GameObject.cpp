//
// Created by TEAMHMG on 26/07/2025.
//

#include "GameObject.h"

UnityEngine::Transform *UnityEngine::GameObject::get_transform() {
    static UnityEngine::Transform *(*get_transform)(UnityEngine::GameObject *) = (UnityEngine::Transform *(*)(UnityEngine::GameObject *)) GET_METHOD("UnityEngine", "GameObject", "get_transform", 0);
    return get_transform(this);
}
