#ifndef SDK_FARMSYSTEM_H
#define SDK_FARMSYSTEM_H

#include "API/Il2CppApi.h"

namespace FarmSystem {
    Class *get_MapMyFarm_class();
    Class *get_MyFarmController_class();
    Class *get_MyFarm_class();
    
    Object *get_MapMyFarm_instance();
    Object *get_MyFarmController_instance();
    
    Object *GetMyFarm();
    List<Object *> *GetFarmList();
    Object *get_CurGearItem();
    void EquipGear(Object *gearItem);
    
    std::vector<Object *> GetMyFarmPlantList(Object *myFarm);
    Object *GetPickablePlant(Object *myFarm, int64_t cropUid, Object *fruitInfo);
    
    std::vector<Object *> GetSlotTransforms(Object *myFarm);
    std::vector<Object *> GetAvailablePlots();
    std::vector<Vector3> GetVirtualSlots(Object *myFarm);
    float GetPlantPointSpacing();
    bool IsPlotAvailable(Object *plot);
    
    Vector3 get_IPickablePlant_Position(Object *plant);
    Vector3 get_Object_Position(Object *obj);
    uint32_t get_IPickablePlant_ItemId(Object *plant);
    Object *get_IPickablePlant_GetPlantData(Object *plant);
    bool get_IPickablePlant_IsPickable(Object *plant);
    
    int64_t get_NetPlantData_CropUid(Object *netPlantData);
    Object *get_NetPlantData_FruitInfo(Object *netPlantData);
    
    void Update();
}

#endif // SDK_FARMSYSTEM_H
