#include "OverlaySnapshot.h"
#include "../FishingCatalog.h"
#include "../SDK/ActorControl.h"
#include "../SDK/CacheUser.h"
#include <API/Il2CppApi.h>
#include <atomic>
#include <Includes/obfuscate.h>

extern bool isGameLoading;

namespace OverlaySnapshot {

namespace {

std::atomic<bool> g_ready{false};
std::atomic<int> g_mapId{0};
std::atomic<float> g_posX{0.f};
std::atomic<float> g_posY{0.f};
std::atomic<float> g_posZ{0.f};

}

void UpdateFromGameThread() {
    if (!il2cpp_loaded.load() || isGameLoading) {
        g_ready.store(false, std::memory_order_release);
        return;
    }
    if (!ActorControl::my_Motor) {
        g_ready.store(false, std::memory_order_release);
        return;
    }
    g_mapId.store(CacheUser::myCurrentMapID(), std::memory_order_relaxed);
    Vector3 pos = ActorControl::my_Motor->invoke_method<Vector3>(OBF("get_TransientPosition"));
    g_posX.store(pos.x, std::memory_order_relaxed);
    g_posY.store(pos.y, std::memory_order_relaxed);
    g_posZ.store(pos.z, std::memory_order_relaxed);
    FishingCatalog::UpdateFromGameThread();
    g_ready.store(true, std::memory_order_release);
}

void Read(View &out) {
    out.ready = g_ready.load(std::memory_order_acquire);
    if (!out.ready) return;
    out.mapId = g_mapId.load(std::memory_order_relaxed);
    out.position.x = g_posX.load(std::memory_order_relaxed);
    out.position.y = g_posY.load(std::memory_order_relaxed);
    out.position.z = g_posZ.load(std::memory_order_relaxed);
}

}
