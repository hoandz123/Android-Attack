//
// Created by TEAMHMG on 12/04/2026.
//

#pragma once
#ifndef IL2CPP_PLAY_AUTOTREASURE_H
#define IL2CPP_PLAY_AUTOTREASURE_H

#include "API/Il2CppApi.h"
#include "API/Vector3.h"
#include <vector>
#include <cmath>
#include <random>

namespace AutoTreasure {

    enum class eAutoTreasure {
        None = 0,
        FindTreasure,
        MovingToTreasure,
        Digging,
        BuyShovel,
        RandomWalk,
        Resting,
        Unstuck
    };

    struct TreasureInfo {
        Object *instance;
        Vector3 position;
        int uid;
        int boxType;
        int64_t ownerUID;
    };

    extern eAutoTreasure curState;
    extern std::vector<TreasureInfo> treasureList;
    extern TreasureInfo currentTarget;
    extern Object *myActor;

    void OnTreasureScan(Object *instance, Vector3 position, int uid, int boxType);
    void RemoveTreasure(int uid);
    void ClearTreasureList();
    void Update(Object *actorInstance);
    void Reset();

    float RandomFloat(float min, float max);
    int RandomInt(int min, int max);

    void DrawESP();
}

#endif //IL2CPP_PLAY_AUTOTREASURE_H
