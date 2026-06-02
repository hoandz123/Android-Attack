#include "OverlaySnapshot.h"
#include "Config/Config.h"
#include "../AutoFishing.h"
#include "../SDK/ActorControl.h"
#include "../SDK/CacheUser.h"
#include "../SDK/enum/eFishingState.h"
#include <API/Il2CppApi.h>
#include <atomic>
#include <Includes/obfuscate.h>

namespace OverlaySnapshot {

namespace {

std::atomic<bool> g_ready{false};
std::atomic<int> g_mapId{0};
std::atomic<float> g_posX{0.f};
std::atomic<float> g_posY{0.f};
std::atomic<float> g_posZ{0.f};
std::atomic<int> g_fishCaught{0};
std::atomic<int> g_fishingState{0};

}

const char *FishingStateLabel(int fishingState) {
    switch ((eFishingState) fishingState) {
        case eFishingState::None: return OBF("Nghỉ");
        case eFishingState::Casting: return OBF("Quăng cần");
        case eFishingState::Search: return OBF("Chờ cắn");
        case eFishingState::SearchResult: return OBF("Kết quả tìm");
        case eFishingState::Idle: return OBF("Chờ");
        case eFishingState::Hit: return OBF("Cắn câu");
        case eFishingState::Fighting: return OBF("Giật cá");
        case eFishingState::Catch: return OBF("Kéo lên");
        case eFishingState::Fail: return OBF("Thất bại");
        case eFishingState::Boast: return OBF("Khoe cá");
        case eFishingState::Finish: return OBF("Xong");
        case eFishingState::CastingFail: return OBF("Quăng lỗi");
        case eFishingState::Miss: return OBF("Trượt");
        case eFishingState::BigFish_RaidEnter: return OBF("Cá lớn vào");
        case eFishingState::BigFish_RaidSync: return OBF("Đồng bộ raid");
        case eFishingState::BigFish_Begin: return OBF("Cá lớn bắt đầu");
        case eFishingState::BigFish_Pumpin: return OBF("Kéo (lớn)");
        case eFishingState::BigFish_Drag: return OBF("Kéo mạnh");
        case eFishingState::BigFish_Tug: return OBF("Giật (lớn)");
        case eFishingState::BigFish_Fighting: return OBF("Chiến (lớn)");
        case eFishingState::BigFish_Catch: return OBF("Bắt (lớn)");
        case eFishingState::BigFish_Miss: return OBF("Trượt (lớn)");
        case eFishingState::BigFish_RaidFighting: return OBF("Raid đánh");
        case eFishingState::BigFish_StunBegin: return OBF("Choáng");
        case eFishingState::BigFish_Stun: return OBF("Choáng giật");
        case eFishingState::BigFish_StunRecovery: return OBF("Hồi choáng");
        default: return OBF("?");
    }
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
    g_fishCaught.store(AutoFishing::GetFishCaughtCount(), std::memory_order_relaxed);
    g_fishingState.store((int) AutoFishing::GetLastFishingState(), std::memory_order_relaxed);
    g_ready.store(true, std::memory_order_release);
}

void Read(View &out) {
    out.ready = g_ready.load(std::memory_order_acquire);
    if (!out.ready) return;
    out.mapId = g_mapId.load(std::memory_order_relaxed);
    out.position.x = g_posX.load(std::memory_order_relaxed);
    out.position.y = g_posY.load(std::memory_order_relaxed);
    out.position.z = g_posZ.load(std::memory_order_relaxed);
    out.fishCaught = g_fishCaught.load(std::memory_order_relaxed);
    out.fishingState = g_fishingState.load(std::memory_order_relaxed);
}

}
