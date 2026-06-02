#include "FarmLandProbe.h"
#include "PlayLog.h"
#include "UnityEngine/Camera.h"
#include "API/Vector3.h"
#include "API/Vector2.h"
#include "FarmSystem.h"
#include "TableSystem.h"
#include "SystemHelper.h"
#include <unordered_map>
#include <algorithm>
#include <cstring>

namespace {
    int64_t get_suid(Object *myFarm) {
        if (!myFarm) return 0;
        if (!myFarm->get_class()->find_method("get_SUID", 0)) return 0;
        return myFarm->invoke_method<int64_t>("get_SUID");
    }

    int get_map_id() {
        Object *map = SystemHelper::get_Map();
        if (!map) return 0;
        if (!map->get_class()->find_method("get_MapId", 0)) return 0;
        return map->invoke_method<int>("get_MapId");
    }

    Vector3 to_world_from_net(Object *netCropInfo) {
        if (!netCropInfo) return Vector3::zero();
        Vector2 pos2 = netCropInfo->get_field_value<Vector2>("<Position>k__BackingField");
        return Vector3(pos2.x, 0, pos2.y);
    }

    static std::unordered_map<int64_t, std::vector<Vector3>> g_landPositions;
    static std::unordered_map<int64_t, FarmLandProbe::LandBounds> g_landBounds;

    void update_bounds(int64_t suid, const Vector3 &pos) {
        if (g_landBounds.find(suid) == g_landBounds.end()) {
            g_landBounds[suid] = FarmLandProbe::LandBounds();
            g_landBounds[suid].suid = suid;
            g_landBounds[suid].minX = pos.x;
            g_landBounds[suid].maxX = pos.x;
            g_landBounds[suid].minZ = pos.z;
            g_landBounds[suid].maxZ = pos.z;
        }
        FarmLandProbe::LandBounds &bounds = g_landBounds[suid];
        bounds.minX = std::min(bounds.minX, pos.x);
        bounds.maxX = std::max(bounds.maxX, pos.x);
        bounds.minZ = std::min(bounds.minZ, pos.z);
        bounds.maxZ = std::max(bounds.maxZ, pos.z);
        bounds.pointCount++;
    }

    void add_position(int64_t suid, const Vector3 &pos) {
        if (suid == 0 || pos == Vector3::zero()) return;
        
        std::vector<Vector3> &positions = g_landPositions[suid];
        bool found = false;
        for (const auto &existing : positions) {
            if (Vector3::Distance(existing, pos) < 0.3f) {
                found = true;
                break;
            }
        }
        if (!found) {
            positions.push_back(pos);
            update_bounds(suid, pos);
        }
    }
}

namespace FarmLandProbe {
    void OnCheckFarmField(Object *handItem, const Vector3 &point, bool isCheckOnly) {
        if (!handItem) return;
        Vector3 goal = handItem->get_field_value<Vector3>("_goalPos");
        bool enter = handItem->get_field_value<bool>("isEnterField");
        std::string name = handItem->get_class() ? handItem->get_class()->get_name() : "null";
        LOGI("FarmLandProbe: CheckFarmField item=%s isCheckOnly=%d point=(%.2f, %.2f, %.2f) enter=%d goal=(%.2f, %.2f, %.2f)",
             name.c_str(), isCheckOnly ? 1 : 0, point.x, point.y, point.z, enter ? 1 : 0, goal.x, goal.y, goal.z);
    }

    void OnPlantSeed(Object *seedItem, uint32_t itemId, const Vector3 &position) {
        std::string name = seedItem && seedItem->get_class() ? seedItem->get_class()->get_name() : "null";
        LOGI("FarmLandProbe: PlantSeed item=%s itemId=%u pos=(%.2f, %.2f, %.2f)", name.c_str(), itemId, position.x, position.y, position.z);
    }

    void OnCreateCrop(Object *myFarm, Object *netCropInfo) {
        int64_t suid = get_suid(myFarm);
        Vector3 wp = to_world_from_net(netCropInfo);
        int64_t cropUid = netCropInfo ? netCropInfo->get_field_value<int64_t>("<CropUid>k__BackingField") : 0;
        uint32_t itemId = netCropInfo ? netCropInfo->get_field_value<uint32_t>("<ItemId>k__BackingField") : 0;
        LOGI("FarmLandProbe: CreateCrop suid=%lld cropUid=%lld itemId=%u pos=(%.2f, %.2f, %.2f)", (long long)suid, (long long)cropUid, itemId, wp.x, wp.y, wp.z);
        add_position(suid, wp);
    }

    void OnUpdatePlant(Object *myFarm, Object *netCropInfo) {
        int64_t suid = get_suid(myFarm);
        Vector3 wp = to_world_from_net(netCropInfo);
        int64_t cropUid = netCropInfo ? netCropInfo->get_field_value<int64_t>("<CropUid>k__BackingField") : 0;
        uint32_t itemId = netCropInfo ? netCropInfo->get_field_value<uint32_t>("<ItemId>k__BackingField") : 0;
        LOGI("FarmLandProbe: UpdatePlant suid=%lld cropUid=%lld itemId=%u pos=(%.2f, %.2f, %.2f)", (long long)suid, (long long)cropUid, itemId, wp.x, wp.y, wp.z);
    }

    void OnRemovePlant(Object *myFarm, int64_t cropUid) {
        int64_t suid = get_suid(myFarm);
        LOGI("FarmLandProbe: RemovePlant suid=%lld cropUid=%lld", (long long)suid, (long long)cropUid);
    }

    void OnCheakGearEnter(Object *myFarm, const Vector3 &worldPos, List<int64_t> *gearList) {
        int64_t suid = get_suid(myFarm);
        int gearCount = gearList ? gearList->get_Count() : 0;
        LOGI("FarmLandProbe: CheakGearEnter suid=%lld pos=(%.2f, %.2f, %.2f) gearCount=%d", (long long)suid, worldPos.x, worldPos.y, worldPos.z, gearCount);
    }

    void OnNetCropInfo(Object *netCropInfo, const char *tag) {
        if (!netCropInfo) return;
        Vector2 pos2 = netCropInfo->get_field_value<Vector2>("<Position>k__BackingField");
        int64_t cropUid = netCropInfo->get_field_value<int64_t>("<CropUid>k__BackingField");
        uint32_t itemId = netCropInfo->get_field_value<uint32_t>("<ItemId>k__BackingField");
        LOGI("FarmLandProbe: NetCropInfo tag=%s cropUid=%lld itemId=%u pos2=(%.2f, %.2f)", tag, (long long)cropUid, itemId, pos2.x, pos2.y);
        
        if (tag && strcmp(tag, "CreatePlantList") == 0) {
            Vector3 wp = Vector3(pos2.x, 0, pos2.y);
            Object *myFarm = FarmSystem::GetMyFarm();
            int64_t suid = get_suid(myFarm);
            add_position(suid, wp);
        }
    }

    void OnGetPlantPointRange(float range, int idx) {
        LOGI("FarmLandProbe: GetPlantPointRange idx=%d range=%.3f", idx, range);
    }

    void OnGetTriggerRadius(float radius) {
        LOGI("FarmLandProbe: GetTriggerRadius radius=%.3f", radius);
    }

    LandBounds GetLandBounds(int64_t suid) {
        if (g_landBounds.find(suid) != g_landBounds.end()) {
            return g_landBounds[suid];
        }
        if (!g_landBounds.empty()) {
            LOGI("FarmLandProbe: SUID %lld not found, using first available bounds", (long long)suid);
            return g_landBounds.begin()->second;
        }
        return LandBounds();
    }

    std::vector<Vector3> GetKnownCropPositions(int64_t suid) {
        if (g_landPositions.find(suid) != g_landPositions.end()) {
            return g_landPositions[suid];
        }
        return std::vector<Vector3>();
    }

    void ClearLandData(int64_t suid) {
        g_landPositions.erase(suid);
        g_landBounds.erase(suid);
    }
}
