#pragma once

#include <string>
#include <API/Il2CppApi.h>

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
        Ing,
        FishingZone,
        Gathering,
        CardCollect,
        Coin,
        NameTag,
        FishBreadShop,
        DragonVillageMonster
    };

    SpawnType GetSpawnType(const std::string &name);
    std::string GetSpawnTypeName(SpawnType type);
    void Update();
}
