#include "AutoFishing.h"
#include "../../Config/Config.h"
#include "../SDK/ActorControl.h"
#include "../SDK/CacheUser.h"
#include "../SDK/FishingSystem.h"
#include "../SDK/SystemHelper.h"
#include "../SDK/TableFishingAreaImpl.h"
#include "../SDK/TableFishingBaitImpl.h"
#include "../SDK/TableFishingDifficultyImpl.h"
#include "../SDK/TableItemImpl.h"
#include "../SDK/enum/eFishingState.h"
#include <API/Il2CppApi.h>
#include <Tools/Tools.h>
#define LOGGER_TAG "ATTACK_AutoFishing"
#include <Includes/Logger.h>

extern bool isGameLoading;

namespace AutoFishing {

// Trampoline gốc IL2CPP (set bởi Hook.cpp)
void (*old_ReceiveFishingCatch)(Object *, Object *);
void (*old_PID_FISHING_CASTING)(Object *, Object *);

namespace {
// Timing / cooldown giữa các action (cast, skip, dialog, đổi mồi)
constexpr int kTickIntervalMs = 400;
constexpr int kActionIntervalMs = 500;
constexpr int kRestartDelayMs = 1500;
constexpr int kStartFishingFailDelayMs = 5000;

// g_time: cooldown | g_sm: FSM | g_cast: cast/telemetry | g_fish: cá đang cắn + catch item | g_filter: skip cycle
struct Timing {
    long long lastActionMs = 0;
    long long lastCastAttemptMs = 0;
    long long lastRewardMs = 0;
    long long lastSkipMs = 0;
    long long lastBaitEquipMs = 0;
};

struct StateMachine {
    eFishingState last = eFishingState::None;
    long long sinceMs = 0;
    bool startFishingFailed = false;
};

struct CastContext {
    unsigned int difficultyId = 0;
    unsigned int castingZoneId = 0;
    unsigned int catchZoneId = 0;
    long long baitUid = 0;
};

struct ActiveFish {
    unsigned int level = 0;
    int shadowIndex = 0;
    unsigned int catchItemId = 0;
};

struct FilterCycle {
    bool skipThisCycle = false;
    unsigned int skipLevel = 0;
    bool tableWarned = false;
};

Timing g_time;
StateMachine g_sm;
CastContext g_cast;
ActiveFish g_fish;
FilterCycle g_filter;

// Chống spam action — cập nhật lastActionMs khi cho phép
bool canActNow() {
    long long now = Tools::getSystemMilliseconds();
    if (g_time.lastActionMs > 0 && now - g_time.lastActionMs < kActionIntervalMs) return false;
    g_time.lastActionMs = now;
    return true;
}

} // anonymous

// Hook cast: lưu FishingDifficultyID từ protocol (fallback level khi chưa có get_FishLevel)
void onPID_FISHING_CASTING(Object *self, Object *protocol) {
    old_PID_FISHING_CASTING(self, protocol);
    if (isGameLoading || !protocol) return;
    g_cast.difficultyId = protocol->invoke_method<unsigned int>(OBF("get_FishingDifficultyID"));
}

// Hook câu trúng: lưu CatchItemID cho kiểm tra bán ở dialog
void onReceiveFishingCatch(Object *self, Object *rewardInfo) {
    old_ReceiveFishingCatch(self, rewardInfo);
    if (isGameLoading || !rewardInfo) return;
    g_fish.catchItemId = rewardInfo->invoke_method<unsigned int>(OBF("get_CatchItemID"));
}

// Tick chính — gọi từ ActorControl::get_Kunit mỗi ~400ms
void Update() {
    // Gate: bật config, không loading, có player/motor/unit, có FishingSystem
    if (!gPLConfig.fishing.enabled || isGameLoading) return;
    if (!ActorControl::my_Player || !ActorControl::my_Motor || !ActorControl::my_Unit) return;
    RATE_LIMIT(kTickIntervalMs);

    Object *player = ActorControl::my_Player;
    Object *fishingSys = SystemHelper::get_Fishing();
    if (!fishingSys) return;

    eFishingState state = player->invoke_method<eFishingState>(OBF("get_FishingState"));

    // Theo dõi đổi state — reset skip cycle, xóa thông tin cá khi bắt đầu cast mới
    if (state != g_sm.last) {
        if (state == eFishingState::None || state == eFishingState::Casting || state == eFishingState::Fail || state == eFishingState::Miss || state == eFishingState::Catch || state == eFishingState::Finish || state == eFishingState::Boast || state == eFishingState::CastingFail) {
            g_filter.skipThisCycle = false;
            g_filter.skipLevel = 0;
            if (state == eFishingState::None || state == eFishingState::Casting) {
                g_fish.level = 0;
                g_fish.shadowIndex = 0;
                g_fish.catchItemId = 0;
            }
        }
        g_sm.last = state;
        g_sm.sinceMs = Tools::getSystemMilliseconds();
    }

    // Telemetry UI + đọc level/bóng cá hiện tại (get_FishLevel hoặc difficultyId cast)
    g_cast.castingZoneId = fishingSys->get_field_value<unsigned int>(OBF("CastingFishingZoneID"));
    g_cast.catchZoneId = fishingSys->invoke_method<unsigned int>(OBF("get_CatchFishingZone"));
    g_cast.baitUid = fishingSys->invoke_method<long long>(OBF("get_FishingBaitUID"));
    if (player->invoke_method<bool>(OBF("get_IsFishing")) && (state == eFishingState::Idle || state == eFishingState::Search || state == eFishingState::Hit || state == eFishingState::Fighting)) {
        g_fish.level = 0;
        g_fish.shadowIndex = 0;
        unsigned int level = player->invoke_method<unsigned int>(OBF("get_FishLevel"));
        if (level == 0) level = g_cast.difficultyId;
        if (level != 0) {
            g_fish.level = level;
            int shadowIdx = 0;
            if (!TableFishingDifficultyImpl::Query(level, &shadowIdx, nullptr)) {
                if ((gPLConfig.fishing.filterByShadow || gPLConfig.fishing.filterByLevel) && !g_filter.tableWarned) {
                    g_filter.tableWarned = true;
                    LOGE(OBF("updateCurrentFishInfo: không resolve FishingDifficulty table"));
                }
            } else {
                g_fish.shadowIndex = shadowIdx;
            }
        }
    }

    // Dialog thưởng: bán theo bóng/grade hoặc đóng tự động
    auto handleRewardDialog = [&]() {
        long long now = Tools::getSystemMilliseconds();
        if (g_time.lastRewardMs > 0 && now - g_time.lastRewardMs < 600) return;
        Object *dlg = nullptr;
        IL2Cpp::Il2CppGetStaticFieldValue(OBF("DialogFishingGetItem"), OBF("Self"), &dlg);
        if (!dlg) return;

        bool shouldSell = false;
        if (gPLConfig.fishing.sellByShadow && g_fish.shadowIndex >= 1 && g_fish.shadowIndex <= 7 &&
            gPLConfig.fishing.sellShadow[g_fish.shadowIndex - 1]) {
            shouldSell = true;
        }
        int grade = (int) fishingSys->invoke_method<int>(OBF("get_CatchItemGrade"));
        if (grade <= 0 && g_fish.catchItemId > 0) grade = TableItemImpl::GetGrade(g_fish.catchItemId);
        if (gPLConfig.fishing.sellByGrade && grade >= 1 && grade <= 5 && gPLConfig.fishing.sellGrade[grade - 1]) {
            shouldSell = true;
        }
        unsigned int itemId = g_fish.catchItemId;
        if (shouldSell && itemId > 0 && !TableItemImpl::GetIsSell(itemId)) shouldSell = false;

        if (shouldSell) {
            if (!canActNow()) return;
            g_time.lastRewardMs = now;
            dlg->invoke_method<void>(OBF("OnClick_Selling"));
            g_fish.catchItemId = 0;
            return;
        }
        if (!gPLConfig.fishing.autoCloseReward) return;
        if (!canActNow()) return;
        g_time.lastRewardMs = now;
        dlg->invoke_method<void>(OBF("OnClick_ButtonClose"));
        g_fish.catchItemId = 0;
    };

    // Dialog đang mở → xử lý ngay (không chờ state Boast/Finish)
    {
        Object *dlg = nullptr;
        IL2Cpp::Il2CppGetStaticFieldValue(OBF("DialogFishingGetItem"), OBF("Self"), &dlg);
        if (dlg) handleRewardDialog();
    }

    // Lọc cá: kiểm tra level (keepLevels) + bóng (keepShadow 1–7)
    const bool filterOn = gPLConfig.fishing.filterByShadow || gPLConfig.fishing.filterByLevel;
    auto keepCurrentFish = [&]() -> bool {
        if (!filterOn || g_fish.level == 0) return true;
        if (gPLConfig.fishing.filterByLevel && !gPLConfig.fishing.keepLevels.empty()) {
            const std::string &s = gPLConfig.fishing.keepLevels;
            bool listed = false;
            for (size_t i = 0; i < s.size();) {
                while (i < s.size() && (s[i] == ' ' || s[i] == ',')) i++;
                unsigned int n = 0;
                bool any = false;
                while (i < s.size() && s[i] >= '0' && s[i] <= '9') {
                    any = true;
                    n = n * 10 + (unsigned int) (s[i++] - '0');
                }
                if (any && n == g_fish.level) listed = true;
                while (i < s.size() && s[i] != ',') i++;
            }
            if (!listed) return false;
        }
        if (gPLConfig.fishing.filterByShadow && g_fish.shadowIndex >= 1 && g_fish.shadowIndex <= 7 && !gPLConfig.fishing.keepShadow[g_fish.shadowIndex - 1]) return false;
        return true;
    };

    // Bỏ cá không muốn: FishLeave (idle/search) hoặc FishingMiss (hit/fighting)
    auto trySkipFish = [&]() {
        if (!filterOn) return;
        if (state != eFishingState::Idle && state != eFishingState::Search && state != eFishingState::Hit && state != eFishingState::Fighting) return;
        if (keepCurrentFish()) return;
        if (g_filter.skipThisCycle && g_filter.skipLevel == g_fish.level) return;
        long long now = Tools::getSystemMilliseconds();
        if (g_time.lastSkipMs > 0 && now - g_time.lastSkipMs < 800) return;
        if (!canActNow()) return;
        g_time.lastSkipMs = now;
        g_filter.skipThisCycle = true;
        g_filter.skipLevel = g_fish.level;
        if (state == eFishingState::Hit || state == eFishingState::Fighting) player->invoke_method<void>(OBF("FishingMiss"));
        else player->invoke_method<void>(OBF("FishLeave"));
        LOGD(OBF("Bỏ cá: lv=%u bóng=%s"), g_fish.level, TableFishingDifficultyImpl::ShadowLabelFromIndex(g_fish.shadowIndex));
    };

    // Cast mới: gắn mồi (smart/config) rồi gọi StartFishing
    auto tryStartFishing = [&]() {
        long long now = Tools::getSystemMilliseconds();
        long long retryDelayMs = g_sm.startFishingFailed ? kStartFishingFailDelayMs : kRestartDelayMs;
        if (g_time.lastCastAttemptMs > 0 && now - g_time.lastCastAttemptMs < retryDelayMs) return;
        if (!player->invoke_method<bool>(OBF("get_HasFishingPole"))) return;
        if (gPLConfig.fishing.autoEquipBait) {
            bool smart = gPLConfig.fishing.smartBaitByZone || gPLConfig.fishing.smartBaitAutoEffect;
            unsigned int baitId = (unsigned int) gPLConfig.fishing.baitItemId;
            if (smart) {
                unsigned int zoneId = FishingSystem::get_CastingFishingZoneID(fishingSys);
                if (zoneId == 0) zoneId = FishingSystem::get_CatchFishingZone(fishingSys);
                unsigned int picked = 0;
                if (gPLConfig.fishing.smartBaitByZone) {
                    for (const auto &entry : gPLConfig.fishing.baitZonePrefs) {
                        if (entry.first == zoneId && entry.second > 0) {
                            picked = entry.second;
                            break;
                        }
                    }
                }
                if (picked == 0 && gPLConfig.fishing.smartBaitAutoEffect) {
                    unsigned int fromFx = TableFishingBaitImpl::PickBestItemIdForAction(TableFishingAreaImpl::GetActionId(zoneId));
                    if (fromFx > 0) picked = fromFx;
                }
                if (picked > 0) baitId = picked;
            }
            if (baitId > 0 && CacheUser::GetItemCount(baitId, true) > 0) {
                long long uid = CacheUser::GetItemUid(baitId);
                if (uid != 0 && FishingSystem::get_FishingBaitUID(fishingSys) != uid) {
                    if (g_time.lastBaitEquipMs == 0 || now - g_time.lastBaitEquipMs >= 1200) {
                        g_time.lastBaitEquipMs = now;
                        FishingSystem::set_FishingBaitUID(fishingSys, uid);
                    }
                }
            }
        }
        if (player->invoke_method<bool>(OBF("get_IsFishing"))) return;
        if (fishingSys->invoke_method<bool>(OBF("get_IsFishingCountOver")) && gPLConfig.fishing.stopWhenCountOver) return;
        if (fishingSys->get_field_value<bool>(OBF("IsFishing")) && state == eFishingState::None) return;
        if (!canActNow()) return;
        g_time.lastCastAttemptMs = now;
        bool started = player->invoke_method<bool>(OBF("StartFishing"));
        g_sm.startFishingFailed = !started;
        if (!started) LOGD(OBF("StartFishing trả về false — đợi %ds"), kStartFishingFailDelayMs / 1000);
    };

    const bool filterBlocking = filterOn && g_filter.skipThisCycle && !keepCurrentFish();

    // Hành vi theo FishingState
    switch (state) {
        case eFishingState::None:
            tryStartFishing();
            break;
        case eFishingState::Casting:
        case eFishingState::SearchResult:
        case eFishingState::Idle:
        case eFishingState::Search:
            trySkipFish();
            break;
        case eFishingState::Hit:
            trySkipFish();
            if (!filterBlocking && canActNow()) player->invoke_method<void>(OBF("FishingHit"));
            break;
        case eFishingState::Fighting:
            if (canActNow()) {
                bool success = true;
                player->invoke_method<void>(OBF("Lift"), success);
            }
            break;
        case eFishingState::Boast:
        case eFishingState::Finish:
            handleRewardDialog();
            break;
        case eFishingState::Fail:
        case eFishingState::Miss:
        case eFishingState::CastingFail:
            if (Tools::getSystemMilliseconds() - g_sm.sinceMs > kRestartDelayMs) tryStartFishing();
            break;
        default:
            break;
    }
}

// Getter telemetry cho PickerSnapshot / UI
unsigned int GetCastingZoneId() { return g_cast.castingZoneId; }
unsigned int GetCatchZoneId() { return g_cast.catchZoneId; }
long long GetFishingBaitUid() { return g_cast.baitUid; }
unsigned int GetCurrentFishLevel() { return g_fish.level; }

}
