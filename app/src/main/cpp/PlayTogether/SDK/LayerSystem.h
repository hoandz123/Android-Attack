//
// Created by TEAMHMG on 13/09/2025.
//

#pragma once
#ifndef IL2CPP_PLAY_LAYERSYSTEM_H
#define IL2CPP_PLAY_LAYERSYSTEM_H

#include <API/Il2CppApi.h>
#include "SystemHelper.h"

namespace LayerSystem {
    Class *get_class();
    Object *get_Instance();
    void ConnectToZoneMove(int targetMapId, int fromType, String *serverName, bool useIris, bool useAdabt);
    void NextMap(int targetMapId);
    void NextMapPos(int mapID, Vector3 pos);
    void Update();
}


#endif //IL2CPP_PLAY_LAYERSYSTEM_H
