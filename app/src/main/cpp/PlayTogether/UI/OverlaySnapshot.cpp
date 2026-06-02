#include "OverlaySnapshot.h"
#include "Config/Config.h"
#include "../AutoFishing.h"
#include "../SDK/ActorControl.h"
#include "../SDK/CacheUser.h"
#include "../SDK/enum/eFishingState.h"
#include <API/Il2CppApi.h>
#include <atomic>
#include <cmath>
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
std::atomic<int> g_failCount{0};
std::atomic<int> g_missCount{0};
std::atomic<int> g_g1{0};
std::atomic<int> g_g2{0};
std::atomic<int> g_g3{0};
std::atomic<int> g_g4{0};
std::atomic<int> g_g5{0};
std::atomic<unsigned int> g_sessionSec{0};
std::atomic<unsigned int> g_lastItemId{0};
std::atomic<int> g_lastGrade{0};
std::atomic<unsigned int> g_castZone{0};
std::atomic<unsigned int> g_catchZone{0};
std::atomic<long long> g_baitUid{0};
std::atomic<bool> g_pausedRare{false};
std::atomic<bool> g_rareAlert{false};
std::atomic<bool> g_hasFloat{false};
std::atomic<float> g_floatOffX{0.f};
std::atomic<float> g_floatOffZ{0.f};
std::atomic<float> g_floatDist{0.f};

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

const char *GradeLabel(int grade) {
    switch (grade) {
        case 1: return OBF("C");
        case 2: return OBF("B");
        case 3: return OBF("A");
        case 4: return OBF("S");
        case 5: return OBF("SS");
        default: return OBF("-");
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
    g_failCount.store(AutoFishing::GetFailCount(), std::memory_order_relaxed);
    g_missCount.store(AutoFishing::GetMissCount(), std::memory_order_relaxed);
    g_g1.store(AutoFishing::GetCatchCountByGrade(1), std::memory_order_relaxed);
    g_g2.store(AutoFishing::GetCatchCountByGrade(2), std::memory_order_relaxed);
    g_g3.store(AutoFishing::GetCatchCountByGrade(3), std::memory_order_relaxed);
    g_g4.store(AutoFishing::GetCatchCountByGrade(4), std::memory_order_relaxed);
    g_g5.store(AutoFishing::GetCatchCountByGrade(5), std::memory_order_relaxed);
    g_sessionSec.store(AutoFishing::GetSessionElapsedSec(), std::memory_order_relaxed);
    g_lastItemId.store(AutoFishing::GetLastCatchItemId(), std::memory_order_relaxed);
    g_lastGrade.store(AutoFishing::GetLastCatchGrade(), std::memory_order_relaxed);
    g_castZone.store(AutoFishing::GetCastingZoneId(), std::memory_order_relaxed);
    g_catchZone.store(AutoFishing::GetCatchZoneId(), std::memory_order_relaxed);
    g_baitUid.store(AutoFishing::GetFishingBaitUid(), std::memory_order_relaxed);
    g_pausedRare.store(AutoFishing::IsPausedByRare(), std::memory_order_relaxed);
    g_rareAlert.store(AutoFishing::HasRareAlert(), std::memory_order_relaxed);
    bool hasFloat = AutoFishing::HasFloatPoint();
    g_hasFloat.store(hasFloat, std::memory_order_relaxed);
    if (hasFloat) {
        Vector3 fp = AutoFishing::GetFloatPoint();
        float dx = fp.x - pos.x;
        float dz = fp.z - pos.z;
        g_floatOffX.store(dx, std::memory_order_relaxed);
        g_floatOffZ.store(dz, std::memory_order_relaxed);
        g_floatDist.store(std::sqrt(dx * dx + dz * dz), std::memory_order_relaxed);
    }
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
    out.failCount = g_failCount.load(std::memory_order_relaxed);
    out.missCount = g_missCount.load(std::memory_order_relaxed);
    out.catchGrade1 = g_g1.load(std::memory_order_relaxed);
    out.catchGrade2 = g_g2.load(std::memory_order_relaxed);
    out.catchGrade3 = g_g3.load(std::memory_order_relaxed);
    out.catchGrade4 = g_g4.load(std::memory_order_relaxed);
    out.catchGrade5 = g_g5.load(std::memory_order_relaxed);
    out.sessionSec = g_sessionSec.load(std::memory_order_relaxed);
    out.lastCatchItemId = g_lastItemId.load(std::memory_order_relaxed);
    out.lastCatchGrade = g_lastGrade.load(std::memory_order_relaxed);
    out.castingZoneId = g_castZone.load(std::memory_order_relaxed);
    out.catchZoneId = g_catchZone.load(std::memory_order_relaxed);
    out.baitUid = g_baitUid.load(std::memory_order_relaxed);
    out.pausedByRare = g_pausedRare.load(std::memory_order_relaxed);
    out.rareAlert = g_rareAlert.load(std::memory_order_relaxed);
    out.hasFloatPoint = g_hasFloat.load(std::memory_order_relaxed);
    out.floatOffsetX = g_floatOffX.load(std::memory_order_relaxed);
    out.floatOffsetZ = g_floatOffZ.load(std::memory_order_relaxed);
    out.floatDistance = g_floatDist.load(std::memory_order_relaxed);
}

}
