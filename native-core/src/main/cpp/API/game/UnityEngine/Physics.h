//
// Created by HMGTEAM on 11/06/2025.
//

#ifndef PLAY_PRO_MAX_PHYSICS_H
#define PLAY_PRO_MAX_PHYSICS_H


#include "API/Vector3.h"
#include "Object.h"
#include "API/Vector2.h"

struct RaycastHit : public System::Object {
public:
    Vector3 m_Point;
    Vector3 m_Normal;
    int m_FaceID;
    float m_Distance;
    Vector2 m_UV;
    int m_Collider;
};


class Physics {
public:
    static bool Raycast(Vector3 origin, Vector3 direction, void* hitInfo);
};


#endif //PLAY_PRO_MAX_PHYSICS_H
