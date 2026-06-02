//
// Created by TEAMHMG on 06/08/2025.
//

#pragma once
#ifndef PLAY_NAVMESHAGENT_H
#define PLAY_NAVMESHAGENT_H


#include "Component.h"
#include "NavMeshPath.h"

class NavMeshAgent : public UnityEngine::Component {
public:

    void SetDestination(Vector3 targetPosition);
    bool CalculatePath(Vector3 targetPosition, NavMeshPath* path);
    int get_areaMask();
};


#endif //PLAY_NAVMESHAGENT_H
