#include "FarmSystem.h"
#include "API/Il2CppApi.h"
#include "SystemHelper.h"
#include "Config/Config.h"
#include "UnityEngine/Camera.h"
#include "Stubs/AutoFarm.h"
#include "Stubs/ESPManager.h"
#include "ActorControl.h"
#include "PlayLog.h"
#include "Tools/Tools.h"
#include "TableSystem.h"
#include "FarmLandProbe.h"
#include <vector>

namespace FarmSystem {
    static Vector3 get_transform_position(Object *transform) {
        if (!transform) return Vector3::zero();
        if (!transform->get_class()->find_method("get_position", 0)) return Vector3::zero();
        return transform->invoke_method<Vector3>("get_position");
    }

    static Vector3 get_hand_point_goal(Object *gear) {
        if (!gear) return Vector3::zero();
        Class *handPointClass = FindClass("HandMyFarmPointItem");
        if (!handPointClass) return Vector3::zero();
        if (!gear->get_class()->is_assignable_from(handPointClass)) return Vector3::zero();
        return gear->get_field_value<Vector3>("_goalPos");
    }

    float GetPlantPointSpacing() {
        Object *tableSys = TableSystem::get_Instance();
        if (!tableSys) return 1.5f;

        Dictionary<int, Object *> *vars = tableSys->get_field_object<Dictionary<int, Object *> *>("Variables");
        if (!vars || vars->get_Count() < 1) return 1.5f;

        Object *vari = vars->get_Item(14);
        if (!vari) return 1.5f;

        if (vari->get_class()->find_method("GetPlantPointRange", 1)) {
            int idx = 0;
            return vari->invoke_method<float>("GetPlantPointRange", idx);
        }

        return 1.5f;
    }

    static Object *find_seed_gear() {
        Object *cur = get_CurGearItem();
        Class *seedClass = FindClass("HandMyFarmSeedItem");
        if (!seedClass) return cur;
        if (cur && cur->get_class()->is_assignable_from(seedClass)) return cur;
        List<Object *> *farmList = GetFarmList();
        if (!farmList) return cur;
        int count = farmList->get_Count();
        for (int i = 0; i < count; i++) {
            Object *item = farmList->get_item(i);
            if (item && item->get_class()->is_assignable_from(seedClass)) {
                EquipGear(item);
                return item;
            }
        }
        return cur;
    }

    Vector3 get_Object_Position(Object *obj) {
        if (!obj) return Vector3::zero();
        
        Class *cls = obj->get_class();
        if (!cls) return Vector3::zero();
        
        std::string clsName = cls->get_name();
        if (clsName != "GameObject" && cls->find_method("get_position", 0)) {
            return obj->invoke_method<Vector3>("get_position");
        }

        if (cls->find_method("get_transform", 0)) {
            Object *transform = obj->invoke_method<Object *>("get_transform");
            return get_transform_position(transform);
        }

        if (cls->find_method("get_Transform", 0)) {
            Object *transform = obj->invoke_method<Object *>("get_Transform");
            return get_transform_position(transform);
        }

        return Vector3::zero();
    }

    Class *get_MapMyFarm_class() {
        return FindClass("MapMyFarm");
    }
    
    Class *get_MyFarmController_class() {
        return FindClass("MyFarmController");
    }
    
    Class *get_MyFarm_class() {
        return FindClass("MyFarm");
    }
    
    Object *get_MapMyFarm_instance() {
        Object *mapInstance = SystemHelper::get_Map();
        if (!mapInstance) {
            LOGI("FarmSystem Debug: mapInstance is null");
            return nullptr;
        }
        
        Object *currentMap = mapInstance->get_field_object<Object *>("m_CurrentMap");
        if (!currentMap) {
        std::string instNameStr = mapInstance->get_class() ? mapInstance->get_class()->get_name() : "null_class";
        const char *instName = instNameStr.c_str();
            LOGI("FarmSystem Debug: currentMap is null from %s.m_CurrentMap", instName);
            return nullptr;
        }
        
        Class *mapMyFarmClass = get_MapMyFarm_class();
        if (!mapMyFarmClass) {
            LOGI("FarmSystem Debug: mapMyFarmClass not found");
            return nullptr;
        }
        
        std::string curNameStr = currentMap->get_class() ? currentMap->get_class()->get_name() : "null_class";
        LOGI("FarmSystem Debug: currentMap class=%s", curNameStr.c_str());
        
        if (currentMap->get_class()->is_assignable_from(mapMyFarmClass)) {
            return currentMap;
        }
        
        LOGI("FarmSystem Debug: currentMap is not MapMyFarm");
        return nullptr;
    }
    
    Object *get_MyFarmController_instance() {
        Object *mapMyFarm = get_MapMyFarm_instance();
        if (!mapMyFarm) return nullptr;
        
        return mapMyFarm->get_field_object<Object *>("Controller");
    }
    
    Object *GetMyFarm() {
        Object *instance = get_MyFarmController_instance();
        if (!instance) return nullptr;
        
        if (!instance->get_class()->find_method("GetMyFarm", 0)) return nullptr;
        return instance->invoke_method<Object *>("GetMyFarm");
    }
    
    List<Object *> *GetFarmList() {
        Object *instance = get_MyFarmController_instance();
        if (!instance) return nullptr;
        
        if (!instance->get_class()->find_method("get_FarmList", 0)) return nullptr;
        return instance->invoke_method<List<Object *> *>("get_FarmList");
    }
    
    Object *get_CurGearItem() {
        Object *instance = get_MyFarmController_instance();
        if (!instance) return nullptr;
        
        if (!instance->get_class()->find_method("get_CurGearItem", 0)) return nullptr;
        return instance->invoke_method<Object *>("get_CurGearItem");
    }
    
    void EquipGear(Object *gearItem) {
        Object *instance = get_MyFarmController_instance();
        if (!instance || !gearItem) return;
        
        if (!instance->get_class()->find_method("EquipGear", 1)) return;
        instance->invoke_method<void>("EquipGear", gearItem);
    }
    
    std::vector<Object *> GetMyFarmPlantList(Object *myFarm) {
        std::vector<Object *> result;
        if (!myFarm) {
            LOGE("GetMyFarmPlantList: myFarm is null");
            return result;
        }

        List<Object *> *allPlants = myFarm->invoke_method<List<Object *> *>("GetAllPlantList");
        if (!allPlants) {
            LOGI("GetMyFarmPlantList: GetAllPlantList() null");
            return result;
        }

        Class *ipickablePlantClass = FindClass("IPickablePlant");
        if (!ipickablePlantClass) {
            LOGI("GetMyFarmPlantList: IPickablePlant class null");
            return result;
        }

        int count = allPlants->get_Count();
        for (int i = 0; i < count; i++) {
            Object *comp = allPlants->get_item(i);
            if (!comp) continue;

            if (!comp->get_class()->is_assignable_from(ipickablePlantClass)) continue;
            result.push_back(comp);
        }

        return result;
    }

    std::vector<Object *> GetSlotTransforms(Object *myFarm) {
        std::vector<Object *> slots;
        if (!myFarm) return slots;

        Object *plantPoint = myFarm->get_field_object<Object *>("PlantPoint");
        if (!plantPoint) {
            LOGI("FarmSystem Debug: PlantPoint is null");
            return slots;
        }

        Object *rootTransform = nullptr;
        if (plantPoint->get_class()->find_method("get_transform", 0)) {
            rootTransform = plantPoint->invoke_method<Object *>("get_transform");
        } else {
            rootTransform = plantPoint;
        }

        if (plantPoint->get_class()->find_method("get_activeInHierarchy", 0)) {
            bool act = plantPoint->invoke_method<bool>("get_activeInHierarchy");
            LOGI("FarmSystem Debug: PlantPoint active=%d", act ? 1 : 0);
        }
        if (rootTransform && rootTransform->get_class()->find_method("get_position", 0)) {
            Vector3 ppos = rootTransform->invoke_method<Vector3>("get_position");
            LOGI("FarmSystem Debug: PlantPoint pos=(%.2f, %.2f, %.2f)", ppos.x, ppos.y, ppos.z);
        }

        if (!rootTransform) return slots;
        if (!rootTransform->get_class()->find_method("get_childCount", 0)) {
            LOGI("FarmSystem Debug: rootTransform missing get_childCount");
            return slots;
        }
        if (!rootTransform->get_class()->find_method("GetChild", 1)) {
            LOGI("FarmSystem Debug: rootTransform missing GetChild");
            return slots;
        }

        int childCount = rootTransform->invoke_method<int>("get_childCount");
        LOGI("FarmSystem Debug: PlantPoint childCount=%d", childCount);
        for (int i = 0; i < childCount; i++) {
            Object *child = rootTransform->invoke_method<Object *>("GetChild", i);
            if (!child) continue;
            slots.push_back(child);
        }

        return slots;
    }
    
    std::vector<Vector3> GetVirtualSlots(Object *myFarm) {
        std::vector<Vector3> slots;
        if (!myFarm) return slots;

        int64_t suid = 0;
        if (myFarm->get_class()->find_method("get_SUID", 0)) {
            suid = myFarm->invoke_method<int64_t>("get_SUID");
        }

        LOGI("FarmSystem: GetVirtualSlots suid=%lld", (long long)suid);
        FarmLandProbe::LandBounds bounds = FarmLandProbe::GetLandBounds(suid);
        LOGI("FarmSystem: GetLandBounds suid=%lld pointCount=%d minX=%.2f maxX=%.2f minZ=%.2f maxZ=%.2f",
             (long long)suid, bounds.pointCount, bounds.minX, bounds.maxX, bounds.minZ, bounds.maxZ);
        
        if (bounds.pointCount > 0) {
            float spacing = GetPlantPointSpacing();
            float margin = spacing * 0.5f;
            int minX = (int)((bounds.minX - margin) / spacing);
            int maxX = (int)((bounds.maxX + margin) / spacing);
            int minZ = (int)((bounds.minZ - margin) / spacing);
            int maxZ = (int)((bounds.maxZ + margin) / spacing);
            
            LOGI("FarmSystem: Using hook-based bounds suid=%lld minX=%.2f maxX=%.2f minZ=%.2f maxZ=%.2f points=%d",
                 (long long)suid, bounds.minX, bounds.maxX, bounds.minZ, bounds.maxZ, bounds.pointCount);
            
            Object *gear = find_seed_gear();
            Class *handPointClass = FindClass("HandMyFarmPointItem");
            bool useRaycast = gear && handPointClass && gear->get_class()->is_assignable_from(handPointClass) && gear->get_class()->find_method("CheckFarmField", 2);
            
            for (int x = minX; x <= maxX; x++) {
                for (int z = minZ; z <= maxZ; z++) {
                    Vector3 probe = Vector3(x * spacing, 0, z * spacing);
                    if (useRaycast) {
                        bool checkOnly = true;
                        gear->invoke_method<void>("CheckFarmField", probe, checkOnly);
                        bool enter = gear->get_field_value<bool>("isEnterField");
                        Vector3 goal = gear->get_field_value<Vector3>("_goalPos");
                        if (enter && goal != Vector3::zero()) {
                            bool dup = false;
                            for (const auto &existing : slots) {
                                if (Vector3::Distance(existing, goal) < 0.3f) {
                                    dup = true;
                                    break;
                                }
                            }
                            if (!dup) slots.push_back(goal);
                        }
                    } else {
                        slots.push_back(probe);
                    }
                }
            }
            
            if (!slots.empty()) {
                LOGI("FarmSystem: Generated %d virtual slots from hook bounds", (int)slots.size());
                return slots;
            }
        }

        Object *plantPoint = myFarm->get_field_object<Object *>("PlantPoint");
        if (!plantPoint) return slots;

        Object *rootTransform = nullptr;
        if (plantPoint->get_class()->find_method("get_transform", 0)) {
            rootTransform = plantPoint->invoke_method<Object *>("get_transform");
        } else {
            rootTransform = plantPoint;
        }

        Vector3 origin = get_Object_Position(rootTransform);
        float spacing = GetPlantPointSpacing();
        Object *gear = find_seed_gear();
        Class *handPointClass = FindClass("HandMyFarmPointItem");
        if (gear && handPointClass && gear->get_class()->is_assignable_from(handPointClass) && gear->get_class()->find_method("CheckFarmField", 2)) {
            static std::vector<Vector3> cache;
            static int64_t cacheSuid = 0;
            if (cacheSuid == suid && !cache.empty()) {
                return cache;
            }
            cache.clear();
            int half = 6;
            for (int dx = -half; dx <= half; dx++) {
                for (int dz = -half; dz <= half; dz++) {
                    Vector3 probe = Vector3(origin.x + dx * spacing, origin.y, origin.z + dz * spacing);
                    bool checkOnly = true;
                    gear->invoke_method<void>("CheckFarmField", probe, checkOnly);
                    bool enter = gear->get_field_value<bool>("isEnterField");
                    Vector3 goal = gear->get_field_value<Vector3>("_goalPos");
                    if (!enter) continue;
                    if (goal == Vector3::zero()) continue;
                    bool dup = false;
                    int cur = (int)cache.size();
                    for (int i = 0; i < cur; i++) {
                        if (Vector3::Distance(cache[i], goal) < 0.3f) {
                            dup = true;
                            break;
                        }
                    }
                    if (!dup) cache.push_back(goal);
                }
            }
            if (!cache.empty()) {
                cacheSuid = suid;
                return cache;
            }
        }

        int half = 3;
        for (int dx = -half; dx <= half; dx++) {
            for (int dz = -half; dz <= half; dz++) {
                Vector3 pos = Vector3(origin.x + dx * spacing, origin.y, origin.z + dz * spacing);
                slots.push_back(pos);
            }
        }

        return slots;
    }

    static void DumpLandSources(Object *myFarm) {
        if (!myFarm) return;
        Object *plantPoint = myFarm->get_field_object<Object *>("PlantPoint");
        Vector3 ppPos = get_Object_Position(plantPoint);
        bool ppActive = plantPoint && plantPoint->get_class()->find_method("get_activeInHierarchy", 0) ? plantPoint->invoke_method<bool>("get_activeInHierarchy") : false;
        int ppChild = 0;
        if (plantPoint) {
            Object *rootTransform = nullptr;
            if (plantPoint->get_class()->find_method("get_transform", 0)) {
                rootTransform = plantPoint->invoke_method<Object *>("get_transform");
            } else {
                rootTransform = plantPoint;
            }
            if (rootTransform && rootTransform->get_class()->find_method("get_childCount", 0)) {
                ppChild = rootTransform->invoke_method<int>("get_childCount");
            }
        }
        float spacing = GetPlantPointSpacing();
        LOGI("FarmSystem Dump: PlantPoint pos=(%.2f, %.2f, %.2f) active=%d child=%d spacing=%.2f", ppPos.x, ppPos.y, ppPos.z, ppActive ? 1 : 0, ppChild, spacing);

        Object *curInfo = nullptr;
        int cropInfoCount = 0;
        if (myFarm->get_class()->find_method("get_CurInfo", 0)) {
            curInfo = myFarm->invoke_method<Object *>("get_CurInfo");
            if (curInfo && curInfo->get_class()->find_method("get_NetPlantCropInfoList", 0)) {
                List<Object *> *cropList = curInfo->invoke_method<List<Object *> *>("get_NetPlantCropInfoList");
                cropInfoCount = cropList ? cropList->get_Count() : 0;
                int logC = cropInfoCount < 5 ? cropInfoCount : 5;
                for (int i = 0; i < logC; i++) {
                    Object *ci = cropList->get_item(i);
                    if (!ci) continue;
                    Vector2 pos = ci->get_field_value<Vector2>("<Position>k__BackingField");
                    int64_t uid = ci->get_field_value<int64_t>("<CropUid>k__BackingField");
                    uint32_t itemId = ci->get_field_value<uint32_t>("<ItemId>k__BackingField");
                    LOGI("FarmSystem Dump: NetPlantCrop idx=%d uid=%lld item=%u pos=(%.2f, %.2f)", i, (long long)uid, itemId, pos.x, pos.y);
                }
            }
        }
        std::vector<Object *> plantList = GetMyFarmPlantList(myFarm);
        LOGI("FarmSystem Dump: plantList size=%d cropInfo=%d", (int)plantList.size(), cropInfoCount);
        int logP = plantList.size() < 5 ? (int)plantList.size() : 5;
        for (int i = 0; i < logP; i++) {
            Object *p = plantList[i];
            if (!p) continue;
            Vector3 pos = get_IPickablePlant_Position(p);
            LOGI("FarmSystem Dump: plant idx=%d pos=(%.2f, %.2f, %.2f)", i, pos.x, pos.y, pos.z);
        }
        std::vector<Vector3> virtualSlots = GetVirtualSlots(myFarm);
        LOGI("FarmSystem Dump: virtualSlots size=%d", (int)virtualSlots.size());
        int logV = virtualSlots.size() < 5 ? (int)virtualSlots.size() : 5;
        for (int i = 0; i < logV; i++) {
            Vector3 pos = virtualSlots[i];
            LOGI("FarmSystem Dump: virtualSlot idx=%d pos=(%.2f, %.2f, %.2f)", i, pos.x, pos.y, pos.z);
        }
        Object *gear = find_seed_gear();
        Vector3 goal = get_hand_point_goal(gear);
        std::string gearNameStr = gear && gear->get_class() ? gear->get_class()->get_name() : "null";
        LOGI("FarmSystem Dump: gear=%s goalPos=(%.2f, %.2f, %.2f)", gearNameStr.c_str(), goal.x, goal.y, goal.z);
    }

    bool IsPlotAvailable(Object *plot) {
        if (!plot) return false;
        
        Class *ipickablePlantClass = FindClass("IPickablePlant");
        if (ipickablePlantClass && plot->get_class()->is_assignable_from(ipickablePlantClass)) {
            Object *plantData = get_IPickablePlant_GetPlantData(plot);
            if (!plantData) return true;
            int64_t cropUid = get_NetPlantData_CropUid(plantData);
            return cropUid == 0;
        }

        return true;
    }
    
    std::vector<Object *> GetAvailablePlots() {
        std::vector<Object *> plots;

        Object *myFarm = GetMyFarm();
        if (!myFarm) return plots;

        std::vector<Object *> slotList = GetSlotTransforms(myFarm);
        std::vector<Object *> plantList = GetMyFarmPlantList(myFarm);

        float occupiedDist = 0.2f;

        LOGI("FarmSystem Debug: slotList count=%d plantList count=%d", (int)slotList.size(), (int)plantList.size());
        if (slotList.empty()) {
            Object *gear = get_CurGearItem();
            Vector3 goal = get_hand_point_goal(gear);
            std::string gearNameStr = gear && gear->get_class() ? gear->get_class()->get_name() : "null";
            LOGI("FarmSystem Debug: slotList empty, gear=%s goalPos=(%.2f, %.2f, %.2f)", gearNameStr.c_str(), goal.x, goal.y, goal.z);
            return plots;
        }

        int slotCount = (int)slotList.size();
        for (int i = 0; i < slotCount; i++) {
            Object *slot = slotList[i];
            if (!slot) continue;

            Vector3 slotPos = get_Object_Position(slot);
            bool occupied = false;

            int plantCount = (int)plantList.size();
            for (int j = 0; j < plantCount; j++) {
                Object *plant = plantList[j];
                if (!plant) continue;
                Vector3 plantPos = get_Object_Position(plant);
                if (Vector3::Distance(slotPos, plantPos) < occupiedDist) {
                    occupied = true;
                    break;
                }
            }

            if (occupied) continue;
            plots.push_back(slot);
        }

        return plots;
    }
    
    Object *GetPickablePlant(Object *myFarm, int64_t cropUid, Object *fruitInfo) {
        if (!myFarm) return nullptr;
        
        if (!myFarm->get_class()->find_method("GetPickablePlant", 2)) return nullptr;
        return myFarm->invoke_method<Object *>("GetPickablePlant", cropUid, fruitInfo);
    }
    
    Vector3 get_IPickablePlant_Position(Object *plant) {
        return get_Object_Position(plant);
    }
    
    uint32_t get_IPickablePlant_ItemId(Object *plant) {
        if (!plant) return 0;
        
        if (!plant->get_class()->find_method("get_ItemId", 0)) return 0;
        return plant->invoke_method<uint32_t>("get_ItemId");
    }
    
    Object *get_IPickablePlant_GetPlantData(Object *plant) {
        if (!plant) return nullptr;
        
        if (!plant->get_class()->find_method("GetPlantData", 0)) return nullptr;
        return plant->invoke_method<Object *>("GetPlantData");
    }
    
    bool get_IPickablePlant_IsPickable(Object *plant) {
        if (!plant) return false;
        
        if (!plant->get_class()->find_method("IsPickable", 0)) return false;
        return plant->invoke_method<bool>("IsPickable");
    }
    
    int64_t get_NetPlantData_CropUid(Object *netPlantData) {
        if (!netPlantData) return 0;
        
        return netPlantData->get_field_value<int64_t>("CropUid");
    }
    
    Object *get_NetPlantData_FruitInfo(Object *netPlantData) {
        if (!netPlantData) return nullptr;
        
        return netPlantData->get_field_object<Object *>("FruitInfo");
    }
    
    void Update() {
        auto &farm = gPLConfig.farm;
        if (!(farm.isAutoPlant || farm.isAutoReap || farm.esp.isEnable)) return;
        if (!ActorControl::my_Motor) return;

        static int64_t lastDumpSuid = 0;
        static bool dumped = false;

        RATE_LIMIT(1000);
        {
            Object *myFarm = FarmSystem::GetMyFarm();
            if (!myFarm) {
                LOGI("FarmSystem Debug: myFarm is null");
            } else {
                int64_t suidReset = 0;
                if (myFarm->get_class()->find_method("get_SUID", 0)) {
                    suidReset = myFarm->invoke_method<int64_t>("get_SUID");
                }
                if (suidReset != lastDumpSuid) {
                    dumped = false;
                    lastDumpSuid = suidReset;
                }
                if (!dumped) {
                    DumpLandSources(myFarm);
                    dumped = true;
                }
                std::string myFarmName = myFarm->get_class() ? myFarm->get_class()->get_name() : "null_class";
                LOGI("FarmSystem Debug: myFarm class=%s", myFarmName.c_str());
                long long suid = 0;
                if (myFarm->get_class()->find_method("get_SUID", 0)) {
                    suid = myFarm->invoke_method<long long>("get_SUID");
                }
                Object *curInfo = nullptr;
                int cropInfoCount = -1;
                int fruitInfoCount = -1;
                int gearInfoCount = -1;
                if (myFarm->get_class()->find_method("get_CurInfo", 0)) {
                    curInfo = myFarm->invoke_method<Object *>("get_CurInfo");
                    if (curInfo) {
                        if (curInfo->get_class()->find_method("get_NetPlantCropInfoList", 0)) {
                            List<Object *> *cropList = curInfo->invoke_method<List<Object *> *>("get_NetPlantCropInfoList");
                            cropInfoCount = cropList ? cropList->get_Count() : 0;
                        }
                        if (curInfo->get_class()->find_method("get_NetPlantFruitInfoList", 0)) {
                            List<Object *> *fruitList = curInfo->invoke_method<List<Object *> *>("get_NetPlantFruitInfoList");
                            fruitInfoCount = fruitList ? fruitList->get_Count() : 0;
                        }
                        if (curInfo->get_class()->find_method("get_NetFarmGearInfoList", 0)) {
                            List<Object *> *gearList = curInfo->invoke_method<List<Object *> *>("get_NetFarmGearInfoList");
                            gearInfoCount = gearList ? gearList->get_Count() : 0;
                        }
                    }
                }
                LOGI("FarmSystem Debug: SUID=%lld CurInfo=%s cropInfo=%d fruitInfo=%d gearInfo=%d",
                     suid,
                     curInfo ? "ok" : "null",
                     cropInfoCount,
                     fruitInfoCount,
                     gearInfoCount);
                std::vector<Object *> slotList = FarmSystem::GetSlotTransforms(myFarm);
                std::vector<Object *> plantList = FarmSystem::GetMyFarmPlantList(myFarm);
                LOGI("FarmSystem Debug: slotList count=%d plantList count=%d", (int)slotList.size(), (int)plantList.size());
                if (slotList.empty()) {
                    Object *gear = get_CurGearItem();
                    Vector3 goal = get_hand_point_goal(gear);
                    std::string gearNameStr = gear && gear->get_class() ? gear->get_class()->get_name() : "null";
                    LOGI("FarmSystem Debug: slotList empty in Update, gear=%s goalPos=(%.2f, %.2f, %.2f)", gearNameStr.c_str(), goal.x, goal.y, goal.z);
                }

                int logSlot = slotList.size() < 5 ? (int)slotList.size() : 5;
                for (int i = 0; i < logSlot; i++) {
                    Object *slot = slotList[i];
                    if (!slot) continue;
                    Vector3 pos = FarmSystem::get_Object_Position(slot);
                    LOGI("FarmSystem Debug: slot idx=%d pos=(%.2f, %.2f, %.2f)", i, pos.x, pos.y, pos.z);
                }

                int logLimit = plantList.size() < 5 ? (int)plantList.size() : 5;
                for (int i = 0; i < logLimit; i++) {
                    Object *plant = plantList[i];
                    if (!plant) continue;

                    Object *plantData = FarmSystem::get_IPickablePlant_GetPlantData(plant);
                    int64_t cropUid = plantData ? FarmSystem::get_NetPlantData_CropUid(plantData) : 0;
                    uint32_t itemId = FarmSystem::get_IPickablePlant_ItemId(plant);
                    bool pickable = FarmSystem::get_IPickablePlant_IsPickable(plant);
                    Vector3 pos = FarmSystem::get_IPickablePlant_Position(plant);

                    LOGI("FarmSystem Debug: idx=%d itemId=%u cropUid=%lld pickable=%d pos=(%.2f, %.2f, %.2f)",
                            i, itemId, (long long)cropUid, pickable ? 1 : 0, pos.x, pos.y, pos.z);
                }
            }
        }

        if (farm.esp.isEnable) {
            Object *myFarm = GetMyFarm();
            if (!myFarm) return;
            
            std::vector<Object *> slotList = GetSlotTransforms(myFarm);
            std::vector<Vector3> virtualSlots;
            bool useVirtual = false;
            if (slotList.empty()) {
                virtualSlots = GetVirtualSlots(myFarm);
                useVirtual = true;
            }

            std::vector<Object *> plantList = GetMyFarmPlantList(myFarm);
            
            UnityEngine::Camera* cam = UnityEngine::Camera::get_main();
            if (!cam) return;
            
            int slotCount = (int)slotList.size();
            for (int i = 0; i < slotCount; i++) {
                Object *slot = slotList[i];
                if (!slot) continue;
                Vector3 pos = get_Object_Position(slot);
                Vector3 screenPos = cam->WorldToScreenPoint(pos);
                if (screenPos.z <= 0) continue;
                std::string name = "Slot";
                ESPManager::Add(slot, (farm.esp.isTeleportButton ? pos : Vector3()), screenPos, name, ImVec4(0, 0.7f, 1, 1));
            }

            if (useVirtual) {
                int vCount = (int)virtualSlots.size();
                for (int i = 0; i < vCount; i++) {
                    Vector3 pos = virtualSlots[i];
                    Vector3 screenPos = cam->WorldToScreenPoint(pos);
                    if (screenPos.z <= 0) continue;
                    std::string name = "SlotV";
                    void *key = reinterpret_cast<void *>((size_t)0x1000 + i);
                    ESPManager::Add(key, (farm.esp.isTeleportButton ? pos : Vector3()), screenPos, name, ImVec4(1, 0.7f, 0, 1));
                }
            }

            int plantCount = (int)plantList.size();
            for (int j = 0; j < plantCount; j++) {
                Object *plant = plantList[j];
                if (!plant) continue;
                
                bool isPickable = get_IPickablePlant_IsPickable(plant);
                if (!isPickable) continue;
                
                Object *plantData = get_IPickablePlant_GetPlantData(plant);
                if (!plantData) continue;
                
                int64_t cropUid = get_NetPlantData_CropUid(plantData);
                uint32_t itemId = get_IPickablePlant_ItemId(plant);
                
                Vector3 pos = get_IPickablePlant_Position(plant);
                Vector3 screenPos = cam->WorldToScreenPoint(pos);
                
                if (screenPos.z <= 0) continue;
                
                std::string name;
                if (farm.esp.isShowName) {
                    name = "Item ID: " + std::to_string(itemId);
                    name += "\nCrop UID: " + std::to_string(cropUid);
                    if (farm.esp.isShowType) {
                        name += "\n[Pickable]";
                    }
                }
                
                ESPManager::Add(plant, (farm.esp.isTeleportButton ? pos : Vector3()), screenPos, name);
            }
        }
        
        if (farm.isAutoPlant || farm.isAutoReap) {
            FarmSys::Update();
        }
    }
}
