#include "AutoFishing.h"
#include "../../Config/Config.h"
#include "PickerSnapshot.h"
#include "../SDK/ActorControl.h"
#include "../SDK/CacheUser.h"
#include "../SDK/CombineContent.h"
#include "../SDK/FishingSystem.h"
#include "../SDK/SystemHelper.h"
#include "../SDK/TableFishingDifficultyImpl.h"
#include "../SDK/TableItemImpl.h"
#include "../SDK/TableRecipeImpl.h"
#include "../SDK/enum/eFishingState.h"
#include <API/Il2CppApi.h>
#include <API/Il2cpp_Struct.h>
#include <Tools/Tools.h>
#define LOGGER_TAG "ATTACK_AutoFishing"
#include <Includes/Logger.h>

extern bool isGameLoading;

namespace AutoFishing {

// Trampoline gốc IL2CPP (set bởi Hook.cpp)
void (*old_ReceiveFishingCatch)(Object *, Object *);
void (*old_PID_FISHING_CASTING)(Object *, Object *);
void (*old_SendToFishingCasting)(Object *, Object *);
void (*old_ShowFishingZoneTitle)(Object *, Object *);

namespace {
// Timing / cooldown giữa các action (cast, skip, dialog, đổi mồi)
constexpr int kTickIntervalMs = 400;
constexpr int kActionIntervalMs = 500;
constexpr int kRestartDelayMs = 1500;
constexpr int kStartFishingFailDelayMs = 5000;
constexpr int kCraftBaitCooldownMs = 500;
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

unsigned int activeFakeZoneId() {
    if (!gPLConfig.fishing.fakeZoneEnabled || gPLConfig.fishing.fakeZoneId == 0) return 0;
    return gPLConfig.fishing.fakeZoneId;
}

void applyFakeZoneClient(unsigned int zoneId) {
    if (zoneId == 0) return;
    Object *fishingSys = SystemHelper::get_Fishing();
    if (!fishingSys) return;
    FishingSystem::set_CastingFishingZoneID(fishingSys, zoneId);
}

void patchFishingZoneList(Object *fishingZoneList, unsigned int zoneId) {
    if (!fishingZoneList || zoneId == 0) return;
    auto *list = (Il2CppList<unsigned int> *) fishingZoneList;
    const int count = list->get_Count();
    if (count <= 0) {
        list->Add(zoneId);
        return;
    }
    for (int i = 0; i < count; i++) list->set_item(i, zoneId);
}

// Hook net send: thay FishingZoneIDList trước khi gửi server
void onSendToFishingCasting(Object *self, Object *fishingZoneList) {
    const unsigned int fakeZone = activeFakeZoneId();
    if (!isGameLoading && fakeZone > 0) {
        patchFishingZoneList(fishingZoneList, fakeZone);
        applyFakeZoneClient(fakeZone);
        LOGD(OBF("Fake zone send: %u"), fakeZone);
    }
    old_SendToFishingCasting(self, fishingZoneList);
}

// Hook title vùng câu: đảm bảo CastingFishingZoneID khớp vùng fake khi hiện title
void onShowFishingZoneTitle(Object *self, Object *command) {
    const unsigned int fakeZone = activeFakeZoneId();
    if (!isGameLoading && fakeZone > 0) applyFakeZoneClient(fakeZone);
    old_ShowFishingZoneTitle(self, command);
}

// Hook cast: lưu FishingDifficultyID từ protocol (fallback level khi chưa có get_FishLevel)
void onPID_FISHING_CASTING(Object *self, Object *protocol) {
    old_PID_FISHING_CASTING(self, protocol);
    if (isGameLoading || !protocol) return;
    g_cast.difficultyId = protocol->invoke_method<unsigned int>(OBF("get_FishingDifficultyID"));
    const unsigned int fakeZone = activeFakeZoneId();
    if (fakeZone > 0) {
        applyFakeZoneClient(fakeZone);
        g_cast.castingZoneId = fakeZone;
    }
}

// Hook câu trúng: lưu CatchItemID cho kiểm tra bán ở dialog
void onReceiveFishingCatch(Object *self, Object *rewardInfo) {
    old_ReceiveFishingCatch(self, rewardInfo);
    if (isGameLoading || !rewardInfo) return;
    g_fish.catchItemId = rewardInfo->invoke_method<unsigned int>(OBF("get_CatchItemID"));
}

// Tick chính — gọi từ ActorControl::get_Kunit mỗi ~400ms
void Update() {
    if (isGameLoading) return;
    const bool wantFish = gPLConfig.fishing.enabled;
    const bool wantBait = gPLConfig.fishing.autoEquipBait;
    const bool wantCraft = gPLConfig.fishing.autoCraftBait;
    if (!wantFish && !wantBait && !wantCraft) return;
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

    // Gắn mồi: lấy baitItemId từ config
    auto resolveBaitItemId = [&]() -> unsigned int {
        return (unsigned int) gPLConfig.fishing.baitItemId;
    };

    // Tự chế mồi — độc lập với gắn mồi (craftBaitItemId / craftBaitTargetCount)
    auto tryAutoCraftBait = [&]() -> bool {
        if (!wantCraft) return false;
        unsigned int baitId = gPLConfig.fishing.craftBaitItemId;
        if (baitId == 0) return false;
        int target = gPLConfig.fishing.craftBaitTargetCount;
        if (target < 1) target = 1;
        int have = CacheUser::GetItemCount(baitId, true);
        if (have >= target) return false;
        long long now = Tools::getSystemMilliseconds();
        if (CombineContent::isCraftInFlight()) return false;
        long long lastAck = CombineContent::lastCraftAckMs();
        if (lastAck > 0 && now - lastAck < kCraftBaitCooldownMs) return false;
        unsigned int recipeId = TableRecipeImpl::ResolveRecipeId(baitId);
        if (recipeId == 0) return false;
        int cookCount = target - have;
        int maxCook = CombineContent::CalculateMaxCookCount(recipeId);
        if (maxCook <= 0) return false;
        if (cookCount > maxCook) cookCount = maxCook;
        if (!CombineContent::TryInstantCombine(recipeId, cookCount, baitId)) return false;
        return true;
    };

    if (wantCraft && tryAutoCraftBait()) return;

    auto tryAutoEquipBait = [&]() -> bool {
        if (!wantBait) return false;
        if (!player->invoke_method<bool>(OBF("get_HasFishingPole"))) return false;
        if (state != eFishingState::None && state != eFishingState::Fail && state != eFishingState::Miss &&
            state != eFishingState::CastingFail) {
            return false;
        }
        long long now = Tools::getSystemMilliseconds();
        unsigned int baitId = resolveBaitItemId();
        if (baitId == 0) return false;
        int count = CacheUser::GetItemCount(baitId, true);
        long long uid = CacheUser::GetBaitItemUid(baitId);
        if (count <= 0 || uid == 0) {
            static long long lastWarnMs = 0;
            if (now - lastWarnMs >= 5000) {
                lastWarnMs = now;
                LOGD(OBF("Gắn mồi thất bại: itemId=%u count=%d uid=%lld"), baitId, count, uid);
            }
            return false;
        }
        if (FishingSystem::get_FishingBaitUID(fishingSys) == uid) return false;
        if (g_time.lastBaitEquipMs > 0 && now - g_time.lastBaitEquipMs < 1200) return false;
        g_time.lastBaitEquipMs = now;
        FishingSystem::set_FishingBaitUID(fishingSys, uid);
        FishingSystem::notifyUpdateFishingBait(fishingSys);
        LOGD(OBF("Gắn mồi: itemId=%u uid=%lld"), baitId, uid);
        return true;
    };

    if (wantBait && tryAutoEquipBait()) {
        g_cast.baitUid = FishingSystem::get_FishingBaitUID(fishingSys);
        return;
    }
    if (!wantFish) {
        g_cast.baitUid = FishingSystem::get_FishingBaitUID(fishingSys);
        return;
    }

    // Telemetry UI + đọc level/bóng cá hiện tại (get_FishLevel hoặc difficultyId cast)
    g_cast.castingZoneId = fishingSys->get_field_value<unsigned int>(OBF("CastingFishingZoneID"));
    if (const unsigned int fakeZone = activeFakeZoneId()) g_cast.castingZoneId = fakeZone;
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

    // Cast mới: gọi StartFishing (mồi đã gắn ở tryAutoEquipBait tick trước)
    auto tryStartFishing = [&]() {
        long long now = Tools::getSystemMilliseconds();
        long long retryDelayMs = g_sm.startFishingFailed ? kStartFishingFailDelayMs : kRestartDelayMs;
        if (g_time.lastCastAttemptMs > 0 && now - g_time.lastCastAttemptMs < retryDelayMs) return;
        if (!player->invoke_method<bool>(OBF("get_HasFishingPole"))) return;
        if (wantBait) {
            unsigned int baitId = resolveBaitItemId();
            if (baitId > 0) {
                long long uid = CacheUser::GetBaitItemUid(baitId);
                if (uid != 0 && FishingSystem::get_FishingBaitUID(fishingSys) != uid) return;
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
