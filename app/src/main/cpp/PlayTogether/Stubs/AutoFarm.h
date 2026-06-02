#pragma once
#ifndef IL2CPP_PLAY_AUTOFARM_H
#define IL2CPP_PLAY_AUTOFARM_H

#include "FarmSystem.h"

namespace FarmSys {
    enum class eAutoState {
        None,
        FindingPlant,
        FindingReap,
        TeleportingToPlant,
        TeleportingToReap,
        Planting,
        Reaping,
        WaitingForPlant,
        WaitingForReap
    };
    
    extern eAutoState currentState;
    extern Object *currentPlant;
    extern Object *currentPlot;
    extern Vector3 posTarget;
    
    Object *FindPlantablePlot();
    Object *FindReapablePlant();
    bool TeleportToPlant(Object *plot);
    bool TeleportToReap(Object *plant);
    bool PlantSeed(Object *plot, uint32_t seedId);
    bool ReapPlant(Object *plant);
    bool isValidPlant(Object *plant);
    bool isValidPlot(Object *plot);
    
    void Update();
}

#endif //IL2CPP_PLAY_AUTOFARM_H
