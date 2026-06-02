#pragma once
#ifndef PLAY_IL2CPP_FARMLANDHOOKS_H
#define PLAY_IL2CPP_FARMLANDHOOKS_H

#include "API/Il2CppApi.h"
#include "API/Vector3.h"

namespace FarmLandHooks {
    void Init();

    extern void (*old_HandMyFarmPointItem_CheckFarmField)(Object *thiz, Vector3 point, bool isCheckOnly);
    void HandMyFarmPointItem_CheckFarmField(Object *thiz, Vector3 point, bool isCheckOnly);

    extern void (*old_HandMyFarmSeedItem_PlantSeed)(Object *thiz, uint32_t itemId, Vector3 position);
    void HandMyFarmSeedItem_PlantSeed(Object *thiz, uint32_t itemId, Vector3 position);

    extern void (*old_MyFarm_CreatePlantList)(Object *thiz, int64_t suid, List<Object *> *cropList, List<Object *> *fruitList, List<int64_t> *affectedGearUidList, bool isShowEffect);
    void MyFarm_CreatePlantList(Object *thiz, int64_t suid, List<Object *> *cropList, List<Object *> *fruitList, List<int64_t> *affectedGearUidList, bool isShowEffect);

    extern void (*old_MyFarm_CreateCrop)(Object *thiz, int64_t suid, Object *info, List<int64_t> *affectedGearUidList, bool isShowEffect);
    void MyFarm_CreateCrop(Object *thiz, int64_t suid, Object *info, List<int64_t> *affectedGearUidList, bool isShowEffect);

    extern void (*old_MyFarm_UpdatePlant_CropList)(Object *thiz, List<Object *> *cropInfoList, List<Object *> *fruitInfoList);
    void MyFarm_UpdatePlant_CropList(Object *thiz, List<Object *> *cropInfoList, List<Object *> *fruitInfoList);

    extern void (*old_MyFarm_UpdatePlant_CropOne)(Object *thiz, Object *cropInfo, List<int64_t> *AffectedGearUidList);
    void MyFarm_UpdatePlant_CropOne(Object *thiz, Object *cropInfo, List<int64_t> *AffectedGearUidList);

    extern void (*old_MyFarm_UpdatePlant_FruitOne)(Object *thiz, Object *fruitInfo, List<int64_t> *AffectedGearUidList);
    void MyFarm_UpdatePlant_FruitOne(Object *thiz, Object *fruitInfo, List<int64_t> *AffectedGearUidList);

    extern void (*old_MyFarm_RemovePlant)(Object *thiz, int64_t cropUid);
    void MyFarm_RemovePlant(Object *thiz, int64_t cropUid);

    extern List<int64_t> *(*old_MyFarm_CheakGearEnter)(Object *thiz, Vector3 worldPos);
    List<int64_t> *MyFarm_CheakGearEnter(Object *thiz, Vector3 worldPos);

    extern float (*old_VariMyFarmDefaultSetting_GetPlantPointRange)(Object *thiz, int idx);
    float VariMyFarmDefaultSetting_GetPlantPointRange(Object *thiz, int idx);

    extern float (*old_VariMyFarmDefaultSetting_GetTriggerRadius)(Object *thiz);
    float VariMyFarmDefaultSetting_GetTriggerRadius(Object *thiz);
}

#endif // PLAY_IL2CPP_FARMLANDHOOKS_H

