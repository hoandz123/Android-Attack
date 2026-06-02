//
// Created by TEAMHMG on 06/08/2025.
//

#pragma once
#ifndef PLAY_NAVMESHPATH_H
#define PLAY_NAVMESHPATH_H


#include "Object.h"
#include "API/Vector3.h"

class NavMeshPath : public System::Object {
public:
    void* m_Ptr;
    Array<Vector3>* m_Corners;
};


#endif //PLAY_NAVMESHPATH_H
