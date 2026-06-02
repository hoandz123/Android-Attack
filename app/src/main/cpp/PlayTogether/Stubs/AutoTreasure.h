#pragma once

#include <API/Il2CppApi.h>
#include <API/Vector3.h>

namespace AutoTreasure {
    void OnTreasureScan(Object *instance, Vector3 position, int uid, int boxType);
    void RemoveTreasure(int uid);
    void Update(Object *actorInstance);
    void DrawESP();
}
