#include "AutoFarm.h"
#include "Config/Config.h"
#include "CacheUser.h"
#include "KinematicCharacterMotor.h"
#include "FarmSystem.h"
#include "Tools/Tools.h"
#include "PlayLog.h"

namespace FarmSys {
    eAutoState currentState = eAutoState::None;
    Object *currentPlant = nullptr;
    Object *currentPlot = nullptr;
    Vector3 currentPlotPos = Vector3::zero();
    Vector3 posTarget = Vector3::zero();
    
    bool isValidPlot(Object *plot) {
        if (!plot) return false;
        
        Class *ipickablePlantClass = FindClass("IPickablePlant");
        if (ipickablePlantClass && plot->get_class()->is_assignable_from(ipickablePlantClass)) {
            Object *plantData = FarmSystem::get_IPickablePlant_GetPlantData(plot);
            if (!plantData) return true;
            
            int64_t cropUid = FarmSystem::get_NetPlantData_CropUid(plantData);
            return cropUid == 0;
        }
        
        return true;
    }
    
    bool isValidPlant(Object *plant) {
        if (!plant) return false;
        
        Class *ipickablePlantClass = FindClass("IPickablePlant");
        if (!ipickablePlantClass) return false;
        if (!plant->get_class()->is_assignable_from(ipickablePlantClass)) return false;
        
        return FarmSystem::get_IPickablePlant_IsPickable(plant);
    }
    
    Object *FindPlantablePlot() {
        auto &farm = gPLConfig.farm;
        if (!farm.isAutoPlant || farm.selectedSeedId == 0) return nullptr;
        
        Object *myFarm = FarmSystem::GetMyFarm();
        if (!myFarm) return nullptr;
        
        std::vector<Object *> slotList = FarmSystem::GetSlotTransforms(myFarm);
        std::vector<Object *> plantList = FarmSystem::GetMyFarmPlantList(myFarm);
        std::vector<Vector3> virtualSlots;
        if (slotList.empty()) {
            virtualSlots = FarmSystem::GetVirtualSlots(myFarm);
        }
        
        LOGI("AutoFarm: FindPlantablePlot slotCount=%d plantCount=%d vCount=%d",
             (int)slotList.size(), (int)plantList.size(), (int)virtualSlots.size());
        
        Object *nearestPlot = nullptr;
        float minDistance = 999999.0f;
        Vector3 playerPos = KinematicCharacterMotor::get_TransientPosition();
        float occupiedDist = 0.2f;
        
        if (!slotList.empty()) {
            int slotCount = (int)slotList.size();
            for (int j = 0; j < slotCount; j++) {
                Object *slot = slotList[j];
                if (!slot) continue;

                if (!farm.selectedPlotPositions.empty()) {
                    auto it = farm.selectedPlotPositions.find(j);
                    if (it == farm.selectedPlotPositions.end() || !it->second) continue;
                }

                if (!isValidPlot(slot)) continue;

                Vector3 slotPos = FarmSystem::get_Object_Position(slot);
                if (slotPos == Vector3::zero()) continue;

                bool occupied = false;
                int plantCount = (int)plantList.size();
                for (int k = 0; k < plantCount; k++) {
                    Object *plant = plantList[k];
                    if (!plant) continue;
                    Vector3 plantPos = FarmSystem::get_Object_Position(plant);
                    if (Vector3::Distance(slotPos, plantPos) < occupiedDist) {
                        occupied = true;
                        break;
                    }
                }
                if (occupied) continue;

                float distance = Vector3::Distance(playerPos, slotPos);
                if (distance < minDistance) {
                    minDistance = distance;
                    nearestPlot = slot;
                    currentPlotPos = slotPos;
                }
            }
        } else {
            int vCount = (int)virtualSlots.size();
            for (int j = 0; j < vCount; j++) {
                Vector3 slotPos = virtualSlots[j];
                if (slotPos == Vector3::zero()) continue;

                float distance = Vector3::Distance(playerPos, slotPos);
                if (distance < minDistance) {
                    minDistance = distance;
                    currentPlotPos = slotPos;
                    nearestPlot = myFarm; // sentinel non-null to trigger state machine
                }
            }
        }

        return nearestPlot;
    }
    
    Object *FindReapablePlant() {
        auto &farm = gPLConfig.farm;
        if (!farm.isAutoReap) return nullptr;
        
        Object *myFarm = FarmSystem::GetMyFarm();
        if (!myFarm) return nullptr;
        
        std::vector<Object *> plantList = FarmSystem::GetMyFarmPlantList(myFarm);
        if (plantList.empty()) return nullptr;
        
        Object *nearestPlant = nullptr;
        float minDistance = 999999.0f;
        Vector3 playerPos = KinematicCharacterMotor::get_TransientPosition();
        
        int plantCount = (int)plantList.size();
        for (int j = 0; j < plantCount; j++) {
            Object *plant = plantList[j];
            if (!plant) continue;
            
            if (!isValidPlant(plant)) continue;
            
            Object *plantData = FarmSystem::get_IPickablePlant_GetPlantData(plant);
            if (!plantData) continue;
            
            int64_t cropUid = FarmSystem::get_NetPlantData_CropUid(plantData);
            Object *fruitInfo = FarmSystem::get_NetPlantData_FruitInfo(plantData);
            
            uint32_t itemId = FarmSystem::get_IPickablePlant_ItemId(plant);
            if (!farm.selectedCropTypes.empty()) {
                auto it = farm.selectedCropTypes.find(itemId);
                if (it == farm.selectedCropTypes.end() || !it->second) continue;
            }
            
            if (!farm.selectedReapPositions.empty()) {
                auto it = farm.selectedReapPositions.find(j);
                if (it == farm.selectedReapPositions.end() || !it->second) continue;
            }
            
            Vector3 plantPos = FarmSystem::get_IPickablePlant_Position(plant);
            float distance = Vector3::Distance(playerPos, plantPos);
            
            if (distance < minDistance) {
                minDistance = distance;
                nearestPlant = plant;
            }
        }
        
        return nearestPlant;
    }
    
    bool TeleportToPlant(Object *plot) {
        Vector3 plotPos = plot ? FarmSystem::get_Object_Position(plot) : currentPlotPos;
        if (plotPos == Vector3::zero()) return false;
        posTarget = plotPos;
        KinematicCharacterMotor::set_TransientPosition(plotPos);
        return true;
    }
    
    bool TeleportToReap(Object *plant) {
        if (!plant) return false;
        
        Vector3 plantPos = FarmSystem::get_IPickablePlant_Position(plant);
        posTarget = plantPos;
        KinematicCharacterMotor::set_TransientPosition(plantPos);
        return true;
    }
    
    bool PlantSeed(Object *plot, uint32_t seedId) {
        if (seedId == 0) return false;
        
        Object *myFarmController = FarmSystem::get_MyFarmController_instance();
        if (!myFarmController) return false;
        
        Object *curGearItem = FarmSystem::get_CurGearItem();
        Class *handMyFarmSeedItemClass = FindClass("HandMyFarmSeedItem");
        if (!handMyFarmSeedItemClass) return false;
        
        if (!curGearItem || !curGearItem->get_class()->is_assignable_from(handMyFarmSeedItemClass)) {
            List<Object *> *farmList = FarmSystem::GetFarmList();
            if (farmList) {
                int count = farmList->get_Count();
                for (int i = 0; i < count; i++) {
                    Object *item = farmList->get_item(i);
                    if (item && item->get_class()->is_assignable_from(handMyFarmSeedItemClass)) {
                        FarmSystem::EquipGear(item);
                        curGearItem = item;
                        break;
                    }
                }
            }
            if (!curGearItem || !curGearItem->get_class()->is_assignable_from(handMyFarmSeedItemClass)) {
                return false;
            }
        }
        
        Vector3 plotPos = plot ? FarmSystem::get_Object_Position(plot) : currentPlotPos;
        if (plotPos == Vector3::zero()) return false;
        Vector2 plotPos2D = Vector2(plotPos.x, plotPos.z);
        
        Class *contentSystemClass = FindClass("ContentSystem");
        if (!contentSystemClass) return false;
        
        Object *myFarmContent = contentSystemClass->get_static_field_value<Object *>("MyFarm");
        if (!myFarmContent) return false;
        
        if (!myFarmContent->get_class()->find_method("SendMyFarmPlantCrop", 4)) return false;
        
        List<int64_t> *affectedGearUidList = nullptr;
        Object *cbPlant = nullptr;
        myFarmContent->invoke_method<void>("SendMyFarmPlantCrop", seedId, plotPos2D, affectedGearUidList, cbPlant);
        
        return true;
    }
    
    bool ReapPlant(Object *plant) {
        if (!plant) return false;
        
        Object *plantData = FarmSystem::get_IPickablePlant_GetPlantData(plant);
        if (!plantData) return false;
        
        int64_t cropUid = FarmSystem::get_NetPlantData_CropUid(plantData);
        Object *fruitInfo = FarmSystem::get_NetPlantData_FruitInfo(plantData);
        
        Class *contentSystemClass = FindClass("ContentSystem");
        if (!contentSystemClass) return false;
        
        Object *myFarmContent = contentSystemClass->get_static_field_value<Object *>("MyFarm");
        if (!myFarmContent) return false;
        
        if (!myFarmContent->get_class()->find_method("SendMyFarmReapCrop", 3)) return false;
        
        Object *cbReap = nullptr;
        myFarmContent->invoke_method<void>("SendMyFarmReapCrop", cropUid, fruitInfo, cbReap);
        
        return true;
    }
    
    void Update() {
        auto &farm = gPLConfig.farm;
        
        switch (currentState) {
            case eAutoState::None: {
                if (farm.isAutoPlant) currentState = eAutoState::FindingPlant;
                if (farm.isAutoReap) currentState = eAutoState::FindingReap;
                break;
            }
            case eAutoState::FindingPlant: {
                if (!farm.isAutoPlant) {
                    currentState = eAutoState::None;
                    return;
                }
                RATE_LIMIT(1000);
                currentPlot = FindPlantablePlot();
                if (currentPlot) {
                    currentState = eAutoState::TeleportingToPlant;
                    LOGI("AutoFarm: Tìm thấy ô đất trống, đang dịch chuyển...");
                } else {
                    currentState = eAutoState::None;
                    LOGI("AutoFarm: Không tìm thấy ô đất trống");
                }
                break;
            }
            case eAutoState::TeleportingToPlant: {
                if (TeleportToPlant(currentPlot)) {
                    currentState = eAutoState::WaitingForPlant;
                    LOGI("AutoFarm: Đã dịch chuyển đến ô đất");
                } else {
                    LOGE("AutoFarm: Không thể dịch chuyển đến ô đất");
                    currentState = eAutoState::FindingPlant;
                }
                break;
            }
            case eAutoState::WaitingForPlant: {
                RATE_LIMIT(1000);
                Vector3 playerPos = KinematicCharacterMotor::get_TransientPosition();
                if (posTarget != Vector3::zero() && Vector3::Distance(playerPos, posTarget) < 2.0f) {
                    posTarget = Vector3::zero();
                    currentState = eAutoState::Planting;
                    LOGI("AutoFarm: Đã đến vị trí ô đất, bắt đầu trồng");
                }
                break;
            }
            case eAutoState::Planting: {
                if (!farm.isAutoPlant) {
                    currentState = eAutoState::None;
                    return;
                }
                RATE_LIMIT(farm.delayPlant);
                if (PlantSeed(currentPlot, farm.selectedSeedId)) {
                    LOGI("AutoFarm: Đã trồng hạt giống");
                    currentPlot = nullptr;
                    currentState = eAutoState::FindingPlant;
                } else {
                    LOGE("AutoFarm: Không thể trồng hạt giống, đang thử lại...");
                    currentState = eAutoState::FindingPlant;
                }
                break;
            }
            case eAutoState::FindingReap: {
                if (!farm.isAutoReap) {
                    currentState = eAutoState::None;
                    return;
                }
                RATE_LIMIT(1000);
                currentPlant = FindReapablePlant();
                if (currentPlant) {
                    currentState = eAutoState::TeleportingToReap;
                    LOGI("AutoFarm: Tìm thấy cây có thể thu hoạch, đang dịch chuyển...");
                } else {
                    currentState = eAutoState::None;
                    LOGI("AutoFarm: Không tìm thấy cây có thể thu hoạch");
                }
                break;
            }
            case eAutoState::TeleportingToReap: {
                if (TeleportToReap(currentPlant)) {
                    currentState = eAutoState::WaitingForReap;
                    LOGI("AutoFarm: Đã dịch chuyển đến cây");
                } else {
                    LOGE("AutoFarm: Không thể dịch chuyển đến cây");
                    currentState = eAutoState::FindingReap;
                }
                break;
            }
            case eAutoState::WaitingForReap: {
                RATE_LIMIT(1000);
                Vector3 playerPos = KinematicCharacterMotor::get_TransientPosition();
                if (posTarget != Vector3::zero() && Vector3::Distance(playerPos, posTarget) < 2.0f) {
                    posTarget = Vector3::zero();
                    currentState = eAutoState::Reaping;
                    LOGI("AutoFarm: Đã đến vị trí cây, bắt đầu thu hoạch");
                }
                break;
            }
            case eAutoState::Reaping: {
                if (!farm.isAutoReap) {
                    currentState = eAutoState::None;
                    return;
                }
                RATE_LIMIT(farm.delayReap);
                if (ReapPlant(currentPlant)) {
                    LOGI("AutoFarm: Đã thu hoạch cây");
                    currentPlant = nullptr;
                    currentState = eAutoState::FindingReap;
                } else {
                    LOGE("AutoFarm: Không thể thu hoạch cây, đang thử lại...");
                    currentState = eAutoState::FindingReap;
                }
                break;
            }
            default:
                currentState = eAutoState::None;
                break;
        }
    }
}
