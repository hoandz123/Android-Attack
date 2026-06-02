//
// Created by TEAMHMG on 13/09/2025.
//

#pragma once
#ifndef IL2CPP_PLAY_KHOIPHUCTRANGTHAI_H
#define IL2CPP_PLAY_KHOIPHUCTRANGTHAI_H
#include <API/Il2CppApi.h>
#include "SDK/enum/Item_Type.h"


namespace KhoiPhucTrangThai {
    struct DataReset {
        int mapID = 0;
        Vector3 pos{};
        long handUID = 0, vehicleUID = 0;
        Item_Type handType = Item_Type::ToolItem, vehicleType = Item_Type::Vehicle;
    } static data;

    enum class State {
        None, Teleport, EquipVehicle, EquipHand, SaveData
    };
    volatile static State st = State::None;
    volatile static int cacheMapId = 0;
    volatile static long long lastMapChangeTime = 0;

    volatile static long long teleportCheckTime = 0;
    static Vector3 teleportTarget{};

    void SystemRestart();

    void Save(long hand, long veh, int map, Vector3 pos);
    bool Load();
    void Update();
}


#endif //IL2CPP_PLAY_KHOIPHUCTRANGTHAI_H
