//
// Created by TEAMHMG on 06/08/2025.
//

#include "NavMeshAgent.h"

void NavMeshAgent::SetDestination(Vector3 targetPosition) {
    static void (*SetDestination)(void*, Vector3) = (void (*)(void*, Vector3)) GET_METHOD("NavMeshAgent", "SetDestination", 1);
    if (SetDestination) {
        SetDestination(this, targetPosition);
    }
}
bool NavMeshAgent::CalculatePath(Vector3 targetPosition, NavMeshPath *path) {
    static bool (*CalculatePath)(void*, Vector3, NavMeshPath*) = (bool (*)(void*, Vector3, NavMeshPath*)) GET_METHOD("NavMeshAgent", "CalculatePath", 2);
    if (CalculatePath) {
        return CalculatePath(this, targetPosition, path);
    }
    return false;
}
int NavMeshAgent::get_areaMask() {
    static int (*get_areaMask)(void*) = (int (*)(void*)) GET_METHOD("NavMeshAgent", "get_areaMask", 0);
    if (get_areaMask) {
        return get_areaMask(this);
    }
    return 0;
}

