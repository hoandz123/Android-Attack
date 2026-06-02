#include "FarmLandHooks.h"
#include "FarmLandProbe.h"
#include "Tools/Tools.h"
#include "PlayLog.h"

namespace FarmLandHooks {
    void (*old_HandMyFarmPointItem_CheckFarmField)(Object *thiz, Vector3 point, bool isCheckOnly);
    void HandMyFarmPointItem_CheckFarmField(Object *thiz, Vector3 point, bool isCheckOnly) {
        if (old_HandMyFarmPointItem_CheckFarmField) {
            old_HandMyFarmPointItem_CheckFarmField(thiz, point, isCheckOnly);
        }
        FarmLandProbe::OnCheckFarmField(thiz, point, isCheckOnly);
    }

    void (*old_HandMyFarmSeedItem_PlantSeed)(Object *thiz, uint32_t itemId, Vector3 position);
    void HandMyFarmSeedItem_PlantSeed(Object *thiz, uint32_t itemId, Vector3 position) {
        FarmLandProbe::OnPlantSeed(thiz, itemId, position);
        if (old_HandMyFarmSeedItem_PlantSeed) {
            old_HandMyFarmSeedItem_PlantSeed(thiz, itemId, position);
        }
    }

    void (*old_MyFarm_CreatePlantList)(Object *thiz, int64_t suid, List<Object *> *cropList, List<Object *> *fruitList, List<int64_t> *affectedGearUidList, bool isShowEffect);
    void MyFarm_CreatePlantList(Object *thiz, int64_t suid, List<Object *> *cropList, List<Object *> *fruitList, List<int64_t> *affectedGearUidList, bool isShowEffect) {
        if (cropList) {
            int count = cropList->get_Count();
            int logC = count < 5 ? count : 5;
            for (int i = 0; i < logC; i++) {
                Object *info = cropList->get_item(i);
                FarmLandProbe::OnNetCropInfo(info, "CreatePlantList");
            }
        }
        if (old_MyFarm_CreatePlantList) {
            old_MyFarm_CreatePlantList(thiz, suid, cropList, fruitList, affectedGearUidList, isShowEffect);
        }
    }

    void (*old_MyFarm_CreateCrop)(Object *thiz, int64_t suid, Object *info, List<int64_t> *affectedGearUidList, bool isShowEffect);
    void MyFarm_CreateCrop(Object *thiz, int64_t suid, Object *info, List<int64_t> *affectedGearUidList, bool isShowEffect) {
        FarmLandProbe::OnCreateCrop(thiz, info);
        if (old_MyFarm_CreateCrop) {
            old_MyFarm_CreateCrop(thiz, suid, info, affectedGearUidList, isShowEffect);
        }
    }

    void (*old_MyFarm_UpdatePlant_CropList)(Object *thiz, List<Object *> *cropInfoList, List<Object *> *fruitInfoList);
    void MyFarm_UpdatePlant_CropList(Object *thiz, List<Object *> *cropInfoList, List<Object *> *fruitInfoList) {
        if (cropInfoList) {
            int count = cropInfoList->get_Count();
            int logC = count < 5 ? count : 5;
            for (int i = 0; i < logC; i++) {
                Object *info = cropInfoList->get_item(i);
                FarmLandProbe::OnNetCropInfo(info, "UpdatePlantList");
            }
        }
        if (old_MyFarm_UpdatePlant_CropList) {
            old_MyFarm_UpdatePlant_CropList(thiz, cropInfoList, fruitInfoList);
        }
    }

    void (*old_MyFarm_UpdatePlant_CropOne)(Object *thiz, Object *cropInfo, List<int64_t> *AffectedGearUidList);
    void MyFarm_UpdatePlant_CropOne(Object *thiz, Object *cropInfo, List<int64_t> *AffectedGearUidList) {
        FarmLandProbe::OnUpdatePlant(thiz, cropInfo);
        if (old_MyFarm_UpdatePlant_CropOne) {
            old_MyFarm_UpdatePlant_CropOne(thiz, cropInfo, AffectedGearUidList);
        }
    }

    void (*old_MyFarm_UpdatePlant_FruitOne)(Object *thiz, Object *fruitInfo, List<int64_t> *AffectedGearUidList);
    void MyFarm_UpdatePlant_FruitOne(Object *thiz, Object *fruitInfo, List<int64_t> *AffectedGearUidList) {
        if (old_MyFarm_UpdatePlant_FruitOne) {
            old_MyFarm_UpdatePlant_FruitOne(thiz, fruitInfo, AffectedGearUidList);
        }
    }

    void (*old_MyFarm_RemovePlant)(Object *thiz, int64_t cropUid);
    void MyFarm_RemovePlant(Object *thiz, int64_t cropUid) {
        FarmLandProbe::OnRemovePlant(thiz, cropUid);
        if (old_MyFarm_RemovePlant) {
            old_MyFarm_RemovePlant(thiz, cropUid);
        }
    }

    List<int64_t> *(*old_MyFarm_CheakGearEnter)(Object *thiz, Vector3 worldPos);
    List<int64_t> *MyFarm_CheakGearEnter(Object *thiz, Vector3 worldPos) {
        List<int64_t> *ret = nullptr;
        if (old_MyFarm_CheakGearEnter) {
            ret = old_MyFarm_CheakGearEnter(thiz, worldPos);
        }
        FarmLandProbe::OnCheakGearEnter(thiz, worldPos, ret);
        return ret;
    }

    float (*old_VariMyFarmDefaultSetting_GetPlantPointRange)(Object *thiz, int idx);
    float VariMyFarmDefaultSetting_GetPlantPointRange(Object *thiz, int idx) {
        float r = 0.0f;
        if (old_VariMyFarmDefaultSetting_GetPlantPointRange) {
            r = old_VariMyFarmDefaultSetting_GetPlantPointRange(thiz, idx);
        }
        FarmLandProbe::OnGetPlantPointRange(r, idx);
        return r;
    }

    float (*old_VariMyFarmDefaultSetting_GetTriggerRadius)(Object *thiz);
    float VariMyFarmDefaultSetting_GetTriggerRadius(Object *thiz) {
        float r = 0.0f;
        if (old_VariMyFarmDefaultSetting_GetTriggerRadius) {
            r = old_VariMyFarmDefaultSetting_GetTriggerRadius(thiz);
        }
        FarmLandProbe::OnGetTriggerRadius(r);
        return r;
    }

    void Init() {
        Class *handPoint = FindClass("HandMyFarmPointItem");
        if (handPoint && handPoint->find_method("CheckFarmField", 2)) {
            Tools::Hook(handPoint->find_method("CheckFarmField", 2)->methodPointer, (void *) HandMyFarmPointItem_CheckFarmField, (void **) &old_HandMyFarmPointItem_CheckFarmField);
            LOGI("FarmLandHooks: Hooked HandMyFarmPointItem.CheckFarmField");
        }

        Class *seed = FindClass("HandMyFarmSeedItem");
        if (seed && seed->find_method("PlantSeed", 2)) {
            Tools::Hook(seed->find_method("PlantSeed", 2)->methodPointer, (void *) HandMyFarmSeedItem_PlantSeed, (void **) &old_HandMyFarmSeedItem_PlantSeed);
            LOGI("FarmLandHooks: Hooked HandMyFarmSeedItem.PlantSeed");
        }

        Class *myFarm = FindClass("MyFarm");
        if (myFarm) {
            if (myFarm->find_method("CreatePlantList", 5)) {
                Tools::Hook(myFarm->find_method("CreatePlantList", 5)->methodPointer, (void *) MyFarm_CreatePlantList, (void **) &old_MyFarm_CreatePlantList);
                LOGI("FarmLandHooks: Hooked MyFarm.CreatePlantList");
            }
            if (myFarm->find_method("CreateCrop", 4)) {
                Tools::Hook(myFarm->find_method("CreateCrop", 4)->methodPointer, (void *) MyFarm_CreateCrop, (void **) &old_MyFarm_CreateCrop);
                LOGI("FarmLandHooks: Hooked MyFarm.CreateCrop");
            }
            if (myFarm->find_method("UpdatePlant", 2)) {
                Tools::Hook(myFarm->find_method("UpdatePlant", 2)->methodPointer, (void *) MyFarm_UpdatePlant_CropList, (void **) &old_MyFarm_UpdatePlant_CropList);
                LOGI("FarmLandHooks: Hooked MyFarm.UpdatePlant(List, List)");
            }
            // Overloads UpdatePlant(NetPlantCropInfo) / UpdatePlant(NetPlantFruitInfo) không phân biệt được bằng API find_method hiện tại,
            // nên tạm thời chỉ hook bản List để giảm rủi ro.
            if (myFarm->find_method("RemovePlant", 1)) {
                Tools::Hook(myFarm->find_method("RemovePlant", 1)->methodPointer, (void *) MyFarm_RemovePlant, (void **) &old_MyFarm_RemovePlant);
                LOGI("FarmLandHooks: Hooked MyFarm.RemovePlant");
            }
            if (myFarm->find_method("CheakGearEnter", 1)) {
                Tools::Hook(myFarm->find_method("CheakGearEnter", 1)->methodPointer, (void *) MyFarm_CheakGearEnter, (void **) &old_MyFarm_CheakGearEnter);
                LOGI("FarmLandHooks: Hooked MyFarm.CheakGearEnter");
            }
        }

        Class *vari = FindClass("VariMyFarmDefaultSetting");
        if (vari) {
            if (vari->find_method("GetPlantPointRange", 1)) {
                Tools::Hook(vari->find_method("GetPlantPointRange", 1)->methodPointer, (void *) VariMyFarmDefaultSetting_GetPlantPointRange, (void **) &old_VariMyFarmDefaultSetting_GetPlantPointRange);
                LOGI("FarmLandHooks: Hooked VariMyFarmDefaultSetting.GetPlantPointRange");
            }
            if (vari->find_method("GetTriggerRadius", 0)) {
                Tools::Hook(vari->find_method("GetTriggerRadius", 0)->methodPointer, (void *) VariMyFarmDefaultSetting_GetTriggerRadius, (void **) &old_VariMyFarmDefaultSetting_GetTriggerRadius);
                LOGI("FarmLandHooks: Hooked VariMyFarmDefaultSetting.GetTriggerRadius");
            }
        }
    }
}

