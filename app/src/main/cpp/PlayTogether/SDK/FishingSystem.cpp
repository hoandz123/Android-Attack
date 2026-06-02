#include "PlayLog.h"
#include "FishingSystem.h"
#include "Config/Config.h"
#include "DialogActionButtons.h"
#include "ActorControl.h"
#include <Tools/Tools.h>
#include "System/DateTime.h"
#include "System/Nullable.h"
#include "SDK/enum/eEquipHandItemType.h"
#include "enum/Item_Type.h"
#include "CacheUser.h"
#include "NetNativeProtocol.h"
#include "NetWebProtocol.h"

namespace FishingSystem {
    int MagicWaterLeft = -1;

    Class *get_class() {
        return FindClass("FishingSystem");
    }

    Object *get_Instance() {
        return SystemHelper::get_Fishing();
    }

    enum class eProductID : int {
        FisherLegend_I_1x = 90164135,  // x1 Nước phép Ngư dân huyền thoại I
        FisherLegend_II_1x = 90164136,  // x1 Nước phép Ngư dân huyền thoại II
        FisherLegend_III_1x = 90164137, // x1 Nước phép Ngư dân huyền thoại III
    };

    enum class eItemID : int {
        FisherLegend_I = 28011137,  // Nước phép Ngư dân huyền thoại I
        FisherLegend_II = 28011138, // Nước phép Ngư dân huyền thoại II
        FisherLegend_III = 28011139 // Nước phép Ngư dân huyền thoại III
    };

    eProductID ProductLv[] = {
            eProductID::FisherLegend_I_1x,
            eProductID::FisherLegend_II_1x,
            eProductID::FisherLegend_III_1x,
    };

    eItemID ItemLv[] = {
            eItemID::FisherLegend_I,
            eItemID::FisherLegend_II,
            eItemID::FisherLegend_III
    };

    Object *AbilitySystem_get_Self() {
        return FindClass("AbilitySystem")->find_method("get_Self", 0)->static_invoke<Object *>();
    }

    bool HasAnyActiveFisherBuff() {
        auto abilitySystem = AbilitySystem_get_Self();
        if (!abilitySystem) {
            LOGE("AbilitySystem is NULL or invalid");
            return false;
        }

        if (abilitySystem->invoke_method<int>("GetCurrentFoodBuffCount") <= 0) {
            return false;
        }

        List<Object *> *bufflist = abilitySystem->invoke_method<List<Object *> *>("GetMyCharacterFoodBuffList");
        if (!bufflist) {
            LOGE("Buff list is empty or NULL");
            return false;
        }

        auto now = System::DateTime::UtcNow();
        long long nowTicks = now.GetTicks();

        int fisherIDs[] = {
                (int) eItemID::FisherLegend_I,
                (int) eItemID::FisherLegend_II,
                (int) eItemID::FisherLegend_III
        };

        for (int i = 0; i < bufflist->get_Count(); i++) {
            Object *buff = bufflist->get_item(i);
            if (!buff) continue;
            int buffItemID = buff->invoke_method<int>("get_ItemID");
            bool isFisherBuff = false;
            for (int j = 0; j < 3; ++j) {
                if (buffItemID == fisherIDs[j]) {
                    isFisherBuff = true;
                    break;
                }
            }
            if (isFisherBuff) {
                auto expireAt = buff->invoke_method<System::Nullable<System::DateTime>>("get_ExpireAt");
                if (expireAt.HasValue()) {
                    auto expired = expireAt.Value();
                    long long ticks = expired.GetTicks();
                    if (ticks > nowTicks) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    int GetActiveBuffCount(int itemID) {
        auto abilitySystem = AbilitySystem_get_Self();
        if (!abilitySystem) {
            LOGE("AbilitySystem is NULL or invalid");
            return 0;
        }

        if (abilitySystem->invoke_method<int>("GetCurrentFoodBuffCount") <= 0) {
            return 0;
        }

        List<Object *> *bufflist = abilitySystem->invoke_method<List<Object *> *>("GetMyCharacterFoodBuffList");
        if (!bufflist) {
            LOGE("Buff list is empty or NULL");
            return 0;
        }

        auto now = System::DateTime::UtcNow();
        long long nowTicks = now.GetTicks();

        int count = 0;
        for (int i = 0; i < bufflist->get_Count(); i++) {
            Object *buff = bufflist->get_item(i);
            if (!buff) continue;
            int buffItemID = buff->invoke_method<int>("get_ItemID");
            if (buffItemID == itemID) {
                auto expireAt = buff->invoke_method<System::Nullable<System::DateTime>>("get_ExpireAt");
                if (expireAt.HasValue()) {
                    auto expired = expireAt.Value();
                    long long ticks = expired.GetTicks();
                    if (ticks > nowTicks) {
                        count++;
                    }
                }
            }
        }
        return count;
    }

    uint64_t get_fishToolUID(Object *fishingSystem) {
        Object *updateFishingBait = fishingSystem->get_field_object<Object *>("UpdateFishingBait");
        if (updateFishingBait) {
            auto target = updateFishingBait->get_field_object<Object *>("m_target");
            if (target) {
                auto itemType = target->invoke_method<eEquipHandItemType>("get_EquipItemType");
                if (itemType == eEquipHandItemType::FishingPole) {
                    return target->invoke_method<uint64_t>("get_UID");
                }
            } else {
                LOGE("get_fishToolUID -> m_Target is null");
            }
        } else {
            LOGE("get_fishToolUID -> UpdateFishingBait is null");
        }
        return 0;
    }
    int getMaxCountMagicWater() {
        int count = 0;
        for (int i = 0; i < 3; ++i) {
            count += gPLConfig.fishing.magicWater.levelUses[i];
        }
        return count;
    }

    void Update() {
        RATE_LIMIT(500);
        Object *instance = get_Instance();
        if (!instance || !gPLConfig.fishing.isCauCa) return;
        if (!ActorControl::my_Motor) return;
        enum class eWaterState {
            Idle,
            EquipWater,
            UseWater,
            EquipRod
        };
        Object *abilitySystem = AbilitySystem_get_Self();
        auto &magicWater = gPLConfig.fishing.magicWater;

        static eWaterState waterState = eWaterState::Idle;
        static uint64_t lastFishToolUID = 0;
        static uint64_t pendingWaterUID = 0;
        static bool isResetToolUID = false;
        static int remainingUses[3] = {0, 0, 0};
        switch (waterState) {
            case eWaterState::Idle: {
                if (instance->get_field_value<bool>("IsFishing")) {
                    lastFishToolUID = get_fishToolUID(instance);
                    isResetToolUID = false;
                    remainingUses[0] = 0;
                    remainingUses[1] = 0;
                    remainingUses[2] = 0;
                    break;
                } else {
                    if (!magicWater.isEnable) {
                        DialogActionButtons::OnClick();
                        break;
                    }
                }
                LOGD("FishingSystem::Update - Idle - fishToolUID=%llu", static_cast<unsigned long long>(lastFishToolUID));
                int buffCount = abilitySystem->invoke_method<int>("GetCurrentFoodBuffCount");
                MagicWaterLeft = buffCount;
                if (lastFishToolUID == 0 || buffCount >= getMaxCountMagicWater()) {
                    if (buffCount >= getMaxCountMagicWater() && isResetToolUID && lastFishToolUID > 0) {
                        waterState = eWaterState::EquipRod;
                        return;
                    }
                    DialogActionButtons::OnClick();
                    break;
                }

                for (int i = 0; i < 3; ++i) {
                    int count = CacheUser::GetCount((int) ItemLv[i]);
                    LOGI("FishingSystem - MagicWater Level %d: buffCount=%d, owned=%d, needed=%d", i + 1, buffCount, count, magicWater.levelUses[i]);
                    if (count < magicWater.levelUses[i]) {
                        NetWebProtocol::RequestToShopBuyList((int) ProductLv[i], magicWater.levelUses[i]);
                    }
                }
                waterState = eWaterState::EquipWater;
                break;
            }
            case eWaterState::EquipWater: {
                RATE_LIMIT(1500);
                for (int i = 0; i < 3; ++i) {
                    int buffCount = remainingUses[i];
                    int currentActiveBuffs = abilitySystem->invoke_method<int>("GetCurrentFoodBuffCount");
                    LOGI("FishingSystem - EquipWater - Checking Level %d: buffCount=%d", i + 1, buffCount);
                    if (magicWater.levelUses[i] > 0 && buffCount < magicWater.levelUses[i] && currentActiveBuffs < getMaxCountMagicWater()) {
                        auto waterItem = CacheUser::GetItem((int) ItemLv[i]);
                        if (waterItem) {
                            static void *nullVal = nullptr;
                            static Item_Type waterType = Item_Type::Food;
                            auto equip = SystemHelper::get_Equip();
                            if (equip) {
                                pendingWaterUID = waterItem->invoke_method<uint64_t>("get_ItemUID");
                                if (pendingWaterUID > 0) {
                                    equip->invoke_method<void>("OnSelectItem", pendingWaterUID, waterType, nullVal);
                                    LOGD("FishingSystem - Equipped water UID=%llu", static_cast<unsigned long long>(pendingWaterUID));
                                    isResetToolUID = true;
                                    waterState = eWaterState::UseWater;
                                    remainingUses[i] += 1;
                                    return;
                                }
                            }
                        }
                    }
                }
                if (isResetToolUID)  {
                    waterState = eWaterState::EquipRod;
                } else {
                    waterState = eWaterState::Idle;
                }
                break;
            }
            case eWaterState::UseWater: {
                RATE_LIMIT(200);
                LOGI("FishingSystem - Using magic water UID=%llu", static_cast<unsigned long long>(pendingWaterUID));
                NetNativeProtocol::SendToItemUse(pendingWaterUID);
                waterState = eWaterState::EquipWater;
                break;
            }
            case eWaterState::EquipRod: {
                RATE_LIMIT(500);
                auto equip = SystemHelper::get_Equip();
                if (equip && lastFishToolUID > 0) {
                    static void *nullVal = nullptr;
                    static Item_Type type = Item_Type::ToolItem;
                    equip->invoke_method<void>("OnSelectItem", lastFishToolUID, type, nullVal);
                    LOGD("FishingSystem - Equipped rod UID=%llu", static_cast<unsigned long long>(lastFishToolUID));
                    isResetToolUID = false;
                }
                waterState = eWaterState::Idle;
                break;
            }
            default: {
                waterState = eWaterState::Idle;
                break;
            }
        }

//    void Update() {
//        RATE_LIMIT(500);
//        MagicWaterLeft = -1;
//        Object *instance = get_Instance();
//        if (!instance || !gPLConfig.fishing.isCauCa) return;
//        if (!ActorControl::my_Motor) return;
//        enum class eWaterState {
//            Idle,
//            EquipWater,
//            UseWater,
//            EquipRod
//        };
//        static eWaterState waterState = eWaterState::Idle;
//        static uint64_t pendingWaterUID = 0;
//        static int remainingUses[3] = {0, 0, 0};
//        static bool initUses = false;
//        static int currentWaterLevel = -1;
//        static bool needWaterAfterStop = false;
//        bool isFishing = instance->get_field_value<bool>("IsFishing");
//        uint64_t fishToolUID = get_fishToolUID(instance);
//        LOGD("FishingSystem::Update - isFishing=%d, fishToolUID=%llu", isFishing, static_cast<unsigned long long>(fishToolUID));
//        if (!isFishing) {
//            auto &magicWater = gPLConfig.fishing.magicWater;
//            if (magicWater.isEnable) {
//                LOGD("FishingSystem - MagicWater enabled");
//                auto equip = SystemHelper::get_Equip();
//                static uint64_t cacheLastFishToolUID = 0;
//                if (cacheLastFishToolUID != fishToolUID && fishToolUID > 0) {
//                    LOGD("FishingSystem - Cache fishToolUID changed: %llu -> %llu", static_cast<unsigned long long>(cacheLastFishToolUID), static_cast<unsigned long long>(fishToolUID));
//                    cacheLastFishToolUID = fishToolUID;
//                }
//                if (cacheLastFishToolUID == 0 && fishToolUID > 0) {
//                    LOGD("FishingSystem - Initial cache fishToolUID: %llu", static_cast<unsigned long long>(fishToolUID));
//                    cacheLastFishToolUID = fishToolUID;
//                }
//
//                if (!initUses) {
//                    for (int i = 0; i < 3; ++i) remainingUses[i] = magicWater.levelUses[i];
//                    LOGD("FishingSystem - Init remainingUses: L1=%d, L2=%d, L3=%d", remainingUses[0], remainingUses[1], remainingUses[2]);
//                    initUses = true;
//                }
//
//                Object *abilitySystem = AbilitySystem_get_Self();
//                if (!abilitySystem) {
//                    LOGE("AbilitySystem is NULL or invalid");
//                    waterState = eWaterState::Idle;
//                    pendingWaterUID = 0;
//                    currentWaterLevel = -1;
//                    needWaterAfterStop = false;
//                    return;
//                }
//
//                int buffCount = abilitySystem->invoke_method<int>("GetCurrentFoodBuffCount");
//                MagicWaterLeft = buffCount;
//                bool hasActiveFisherBuff = HasAnyActiveFisherBuff();
//                LOGD("FishingSystem - buffCount=%d, hasActiveFisherBuff=%d, waterState=%d, needWaterAfterStop=%d", buffCount, hasActiveFisherBuff, (int) waterState, needWaterAfterStop);
//
//                if (buffCount >= 5) {
//                    LOGD("FishingSystem - buffCount >= 5, reset state and equip rod");
//                    waterState = eWaterState::Idle;
//                    pendingWaterUID = 0;
//                    currentWaterLevel = -1;
//                    needWaterAfterStop = false;
//                    if (cacheLastFishToolUID && fishToolUID < 1 && equip) {
//                        static void *nullVal = nullptr;
//                        static Item_Type type = Item_Type::ToolItem;
//                        equip->invoke_method<void>("OnSelectItem", cacheLastFishToolUID, type, nullVal);
//                        LOGD("FishingSystem - Equipped rod UID=%llu (buffCount >= 5)", static_cast<unsigned long long>(cacheLastFishToolUID));
//                    }
//                    DialogActionButtons::OnClick();
//                    return;
//                }
//
//                if (buffCount < 5 && (!hasActiveFisherBuff || needWaterAfterStop)) {
//                    if (fishToolUID > 0 || cacheLastFishToolUID > 0) {
//                        int levelIdx = -1;
//                        int levelCount = 0;
//                        for (int i = 2; i >= 0; --i) {
//                            int count = CacheUser::GetCount((int) ItemLv[i]);
//                            if (count > 0) {
//                                if (remainingUses[i] < 1) remainingUses[i] = magicWater.levelUses[i];
//                                levelIdx = i;
//                                levelCount = count;
//                                break;
//                            }
//                        }
//                        LOGD("FishingSystem - Selected levelIdx=%d, remainingUses: L1=%d, L2=%d, L3=%d", levelIdx, remainingUses[0], remainingUses[1], remainingUses[2]);
//                        if (levelIdx < 0) {
//                            LOGD("FishingSystem - No water in bag, buying level 3");
//                            RATE_LIMIT(2000);
//                            waterState = eWaterState::Idle;
//                            pendingWaterUID = 0;
//                            currentWaterLevel = -1;
//                            needWaterAfterStop = false;
//                            NetWebProtocol::RequestToShopBuyList((int) ProductLv[2], 6);
//                            DialogActionButtons::OnClick();
//                            return;
//                        }
//                        int isWaterCount = levelCount;
//
//                        auto waterItem = CacheUser::GetItem((int) ItemLv[levelIdx]);
//                        if (!waterItem) {
//                            LOGE("Water item not found for level %d", levelIdx + 1);
//                            waterState = eWaterState::Idle;
//                            pendingWaterUID = 0;
//                            currentWaterLevel = -1;
//                            needWaterAfterStop = false;
//                            return;
//                        }
//
//                        if (!equip) {
//                            LOGE("FishingSystem - Equip system is NULL");
//                            waterState = eWaterState::Idle;
//                            pendingWaterUID = 0;
//                            currentWaterLevel = -1;
//                            needWaterAfterStop = false;
//                            return;
//                        }
//
//                        static void *nullVal = nullptr;
//                        static Item_Type waterType = Item_Type::Food;
//                        static Item_Type toolType = Item_Type::ToolItem;
//
//                        switch (waterState) {
//                            case eWaterState::Idle: {
//                                currentWaterLevel = levelIdx;
//                                pendingWaterUID = waterItem->invoke_method<uint64_t>("get_ItemUID");
//                                LOGD("FishingSystem - State Idle: levelIdx=%d, pendingWaterUID=%llu", levelIdx, static_cast<unsigned long long>(pendingWaterUID));
//                                if (!pendingWaterUID) {
//                                    LOGE("FishingSystem - Failed to get water item UID");
//                                    waterState = eWaterState::Idle;
//                                    currentWaterLevel = -1;
//                                    needWaterAfterStop = false;
//                                    return;
//                                }
//                                needWaterAfterStop = false;
//                                waterState = eWaterState::EquipWater;
//                                return;
//                            }
//                            case eWaterState::EquipWater: {
//                                RATE_LIMIT(300);
//                                LOGD("FishingSystem - State EquipWater: pendingWaterUID=%llu", static_cast<unsigned long long>(pendingWaterUID));
//                                if (pendingWaterUID) {
//                                    equip->invoke_method<void>("OnSelectItem", pendingWaterUID, waterType, nullVal);
//                                    LOGD("FishingSystem - Called OnSelectItem for water UID=%llu", static_cast<unsigned long long>(pendingWaterUID));
//                                }
//                                waterState = eWaterState::UseWater;
//                                return;
//                            }
//                            case eWaterState::UseWater: {
//                                RATE_LIMIT(500);
//                                LOGD("FishingSystem - State UseWater: pendingWaterUID=%llu, level=%d", static_cast<unsigned long long>(pendingWaterUID), currentWaterLevel);
//                                if (pendingWaterUID) {
//                                    NetNativeProtocol::SendToItemUse(pendingWaterUID);
//                                    LOGD("FishingSystem - Called SendToItemUse for water UID=%llu", static_cast<unsigned long long>(pendingWaterUID));
//                                    if (currentWaterLevel >= 0 && currentWaterLevel < 3 && remainingUses[currentWaterLevel] > 0) {
//                                        remainingUses[currentWaterLevel]--;
//                                        LOGD("FishingSystem - Used water level %d, remaining: %d", currentWaterLevel + 1, remainingUses[currentWaterLevel]);
//                                    }
//                                }
//                                waterState = eWaterState::EquipRod;
//                                return;
//                            }
//                            case eWaterState::EquipRod: {
//                                RATE_LIMIT(300);
//                                LOGD("FishingSystem - State EquipRod: cacheLastFishToolUID=%llu", static_cast<unsigned long long>(cacheLastFishToolUID));
//                                if (cacheLastFishToolUID) {
//                                    equip->invoke_method<void>("OnSelectItem", cacheLastFishToolUID, toolType, nullVal);
//                                    LOGD("FishingSystem - Called OnSelectItem for rod UID=%llu", static_cast<unsigned long long>(cacheLastFishToolUID));
//                                }
//                                waterState = eWaterState::Idle;
//                                pendingWaterUID = 0;
//                                currentWaterLevel = -1;
//                                return;
//                            }
//                            default: {
//                                LOGD("FishingSystem - State default, reset to Idle");
//                                waterState = eWaterState::Idle;
//                                pendingWaterUID = 0;
//                                currentWaterLevel = -1;
//                                return;
//                            }
//                        }
//                    } else {
//                        LOGE("FishingSystem - No fishing tool equipped");
//                    }
//                }
//            }
//            DialogActionButtons::OnClick();
//            return;
//        } else {
//            int fishZone = instance->get_field_value<int>("CastingFishingZoneID");
//            PLConfig::FishingConfig::curFishZone = fishZone;
//            LOGD("FishingSystem - Currently fishing, zoneID=%d", fishZone);
//
//            auto &magicWater = gPLConfig.fishing.magicWater;
//            if (magicWater.isEnable) {
//                Object *abilitySystem = AbilitySystem_get_Self();
//                if (abilitySystem) {
//                    int buffCount = abilitySystem->invoke_method<int>("GetCurrentFoodBuffCount");
//                    MagicWaterLeft = buffCount;
//                    bool hasActiveFisherBuff = HasAnyActiveFisherBuff();
//                    LOGD("FishingSystem - While fishing: buffCount=%d, hasActiveFisherBuff=%d", buffCount, hasActiveFisherBuff);
//
//                    if (buffCount < 5 && !hasActiveFisherBuff) {
//                        LOGD("FishingSystem - Buff expired while fishing, will use water when stop fishing");
//                        waterState = eWaterState::Idle;
//                        pendingWaterUID = 0;
//                        currentWaterLevel = -1;
//                        needWaterAfterStop = true;
//                        DialogActionButtons::OnClick();
//                    }
//                }
//            }
//        }
    }

}