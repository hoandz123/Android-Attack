//
// Created by TEAMHMG on 10/09/2025.
//

#pragma once
#ifndef IL2CPP_PLAY_AUTOCOLLECT_H
#define IL2CPP_PLAY_AUTOCOLLECT_H

#include <string>
#include <vector>
#include <algorithm>
#include "CollectSystem.h"

namespace CollectSys {
    enum class SpawnType {
        Unknown,
        Vein,
        Plants,
        GemVein,
        Fossil,
        Slime,
        Snowman,
        Ore,
        Ing,          // Ingredients (e.g., spawn_ing_...)
        FishingZone,
        Gathering,    // e.g., spawn_gathering_...
        CardCollect,  // e.g., spawn_cardcollect_...
        Coin,         // e.g., spawn_coin_...
        NameTag,      // e.g., spawn_nametag_...
        FishBreadShop,
        DragonVillageMonster
    };

    SpawnType GetSpawnType(const std::string &name);
    bool IsObjectOfType(const std::string &name, SpawnType type);
    std::string GetSpawnTypeName(SpawnType type);

    enum class eAutoState {
        None,
        FindingCollect,
        TeleportingCollect,
        WaitingForCollect,
        FindingVein,
        Teleporting,
        WaitingForTeleport,
        CatchingVein,
        NextMap,
        WaitingForNextMap,
    };

    extern eAutoState currentState;
    extern Object *currentVein;
    extern Object *currentCollect;
    extern int targetMapId;

    Object *FindCollect();
    bool TeleportToCollect();
    bool isValidCollect(Object *collect);

    Object *FindVein();
    bool TeleportToVein();
    bool isCatchVein();
    bool isValidVein(Object *vein);
    extern Vector3 posTarget;

    void Update();
}

#endif //IL2CPP_PLAY_AUTOCOLLECT_H
