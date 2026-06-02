#pragma once
#ifndef PLAY_IL2CPP_FARMLANDPROBE_H
#define PLAY_IL2CPP_FARMLANDPROBE_H

#include "API/Il2CppApi.h"
#include "API/Vector3.h"
#include "API/Vector2.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>

namespace FarmLandProbe {
    void OnCheckFarmField(Object *handItem, const Vector3 &point, bool isCheckOnly);
    void OnPlantSeed(Object *seedItem, uint32_t itemId, const Vector3 &position);
    void OnCreateCrop(Object *myFarm, Object *netCropInfo);
    void OnUpdatePlant(Object *myFarm, Object *netCropInfo);
    void OnRemovePlant(Object *myFarm, int64_t cropUid);
    void OnCheakGearEnter(Object *myFarm, const Vector3 &worldPos, List<int64_t> *gearList);
    void OnNetCropInfo(Object *netCropInfo, const char *tag);
    void OnGetPlantPointRange(float range, int idx);
    void OnGetTriggerRadius(float radius);
    
    struct LandBounds {
        float minX, maxX, minZ, maxZ;
        int pointCount;
        int64_t suid;
        LandBounds() : minX(0), maxX(0), minZ(0), maxZ(0), pointCount(0), suid(0) {}
    };
    
    LandBounds GetLandBounds(int64_t suid);
    std::vector<Vector3> GetKnownCropPositions(int64_t suid);
    void ClearLandData(int64_t suid);
}

#endif // PLAY_IL2CPP_FARMLANDPROBE_H
