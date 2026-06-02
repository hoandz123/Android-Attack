//
// Created by TEAMHMG on 26/07/2025.
//

#pragma once
#ifndef PLAY_GAMEOBJECT_H
#define PLAY_GAMEOBJECT_H

#include "API/Il2CppApi.h"
#include "Object.h"
#include "Transform.h"

namespace UnityEngine {
    class GameObject : public UnityEngine::Object {
    public:
        UnityEngine::Transform* get_transform();
    };

}
#endif //PLAY_GAMEOBJECT_H
