#include "PlayLog.h"
//
// Created by TEAMHMG on 13/09/2025.
//

#include "ActorDefaultControlPlayer.h"
#include "TableSystem.h"
#include "Config/Config.h"
#include "ActorControl.h"
#include "enum/eFishingState.h"
#include "DialogJoyStick.h"
#include "FrameWork.h"
#include "KhoiPhucTrangThai.h"
#include "TableFishingDifficultyImpl.h"
#include "TableSystem.h"
#include "enum/eTableType.h"
#include <Tools/Tools.h>
#include <map>
#include <cstdint>
#include "UnityEngine/Object.h"
#include "UnityEngine/Component.h"

namespace ActorDefaultControlPlayer { //Bóng cá
    Class *get_class() {
        return FindClass("ActorDefaultControlPlayer");
    }

    std::map<void*, int> originalValues;

    void SetNewZone(int zoneId) {
        Array<void **> *list = UnityEngine::Component::FindObjectsOfTypeAll(UnityEngine::Component::GetType(String::Create("FisheryZoneTrigger, Assembly-CSharp")));
        if (list) {
            for (int i = 0; i < list->getLength(); i++) {
                void *it = list->getPointer()[i];
                if (it) {
                    uintptr_t offset = (uintptr_t)it + IL2Cpp::Il2CppGetFieldOffset("FisheryZoneTrigger", "FishingZoneID");
                    int *valuePtr = (int*)offset;

                    if (originalValues.find(it) == originalValues.end()) {
                        originalValues[it] = *valuePtr;
                    }

                    *valuePtr = zoneId;
                    LOGD("SetNewZone: Set zone %d for trigger %p", zoneId, it);
                }
            }
        }
    }

    void RestoreZone() {
        Array<void **> *list = UnityEngine::Component::FindObjectsOfTypeAll(UnityEngine::Component::GetType(String::Create("FisheryZoneTrigger, Assembly-CSharp")));
        if (list) {
            for (int i = 0; i < list->getLength(); i++) {
                void *it = list->getPointer()[i];
                if (it) {
                    uintptr_t offset = (uintptr_t)it + IL2Cpp::Il2CppGetFieldOffset("FisheryZoneTrigger", "FishingZoneID");
                    int *valuePtr = (int*)offset;
                    auto it2 = originalValues.find(it);
                    if (it2 != originalValues.end()) {
                        *valuePtr = it2->second;
                        LOGD("RestoreZone: Restored zone %d for trigger %p", it2->second, it);
                    }
                }
            }
        }
    }

    static uint32_t EffectiveFishingZoneId() {
        if (gPLConfig.fishing.isFakeVR)
            return 503u;
        if (gPLConfig.fishing.isFishZone && gPLConfig.fishing.fishZone > 0)
            return static_cast<uint32_t>(gPLConfig.fishing.fishZone);
        return 0u;
    }

    static Object *FishingArea_FindByZoneId(uint32_t zoneId) {
        Object *table = TableSystem::GetTableUnit<Object *>(
                FindClass("PlayTogether.Table.eTableType")->get_enum_value<eTableType>("FishingArea"));
        if (!table)
            return nullptr;
        return table->invoke_method<Object *>("GetTableData", zoneId);
    }

    void (*old_SendToFishingCasting)(void *, void *);

    void Hook_SendToFishingCasting(void *thiz, void *fishingZoneList) {
        if (fishingZoneList) {
            auto *list = reinterpret_cast<List<uint32_t> *>(fishingZoneList);
            const int n = list->get_Count();
            const uint32_t replaceZone = EffectiveFishingZoneId();
            LOGI("[SendToFishingCasting] count=%d replaceZone=%u (FakeVR=%d fishZone=%d isFishZone=%d)",
                 n, replaceZone, gPLConfig.fishing.isFakeVR ? 1 : 0, gPLConfig.fishing.fishZone,
                 gPLConfig.fishing.isFishZone ? 1 : 0);
            for (int i = 0; i < n; i++) {
                uint32_t v = list->get_item(i);
                if (v > 0u && replaceZone > 0u) {
                    LOGI("[SendToFishingCasting]  [%d] %u -> %u", i, v, replaceZone);
                    list->set_item(i, replaceZone);
                }
            }
        } else {
            LOGI("[SendToFishingCasting] list=null");
        }
        old_SendToFishingCasting(thiz, fishingZoneList);
    }

    void (*old_DialogZoneTitle_SetTitle)(void *, uint32_t);

    void Hook_DialogZoneTitle_SetTitle(void *thiz, uint32_t titleID) {
        uint32_t id = titleID;
        const unsigned z = EffectiveFishingZoneId();
        if (z > 0u) {
            if (Object *row = FishingArea_FindByZoneId(static_cast<uint32_t>(z))) {
                const uint32_t textId = row->invoke_method<uint32_t>("get_FishingZoneText");
                if (textId > 0u) {
                    id = textId;
                    LOGI("[DialogZoneTitle.SetTitle] zone=%u titleID %u -> %u (FishingZoneText)", z, titleID, id);
                }
            }
        }
        old_DialogZoneTitle_SetTitle(thiz, id);
    }
    int GetFishShadowLevel(int level) {
        Object *difficulty = TableFishingDifficultyImpl::GetTableData(level);
        if (!difficulty) {
            LOGE("GetFishShadowLevel: difficulty is null for level %d", level);
            LOGE("GetFishShadowLevel: difficulty is null for level %d", level);
            return 0;
        }

        String *AssetName = difficulty->get_field_object<String *>("<AssetName>k__BackingField");
        if (!AssetName) {
            LOGE("GetFishShadowLevel: AssetName is null for level %d", level);
            LOGE("GetFishShadowLevel: AssetName is null for level %d", level);
            return 0;
        }
        std::string fishShadow = AssetName->to_string();
        std::string keywords[] = {
                "fish_s_shadow", "fish_m_shadow", "fish_l_shadow",
                "fish_xl_shadow", "fish_xxl_shadow", "fish_xxxl_shadow",
                "fish_4xl_shadow"
        };

        for (int i = 0; i < 7; i++) { // lấy bóng cá hiện tại
            if (fishShadow.find(keywords[i]) != std::string::npos) {
                return i + 1;
                break;
            }
        }
        LOGE("GetFishShadowLevel: No matching shadow found for AssetName %s", fishShadow.c_str());
        LOGE("GetFishShadowLevel: No matching shadow found for AssetName %s", fishShadow.c_str());
        return 0;
    }


    void Update() {
        RATE_LIMIT(60);
        Object *instance = ActorControl::my_Player;
        if (!instance || !gPLConfig.fishing.isCauCa) return;

        static int lastFishLevel = 0;
        eFishingState state = instance->invoke_method<eFishingState>("get_FishingState");
        switch (state) {
            case eFishingState::None: {
                lastFishLevel = 0;
                break;
            }
            case eFishingState::Idle: {
                PLConfig::FishingConfig::curFishLevel = instance->invoke_method<int>("get_FishLevel");
                if (PLConfig::FishingConfig::curFishLevel <= 0) {
                    LOGE("ActorDefaultControlPlayer::Idle - fishLevel is invalid: %d", PLConfig::FishingConfig::curFishLevel);
                    DialogJoyStick::OnPress_JumpButton();
                    return;
                }
                PLConfig::FishingConfig::curFishShadowLevel = GetFishShadowLevel(PLConfig::FishingConfig::curFishLevel);
                if (PLConfig::FishingConfig::curFishShadowLevel <= 0) {
                    LOGE("ActorDefaultControlPlayer::Idle - fishShadowLevel is invalid: %d", PLConfig::FishingConfig::curFishShadowLevel);
                    DialogJoyStick::OnPress_JumpButton();
                    return;
                }
                bool isFilterID = false;
                if (gPLConfig.fishing.isLocID) {
                    for (auto &e: gPLConfig.fishing.IDLocCa) {
                        if (e.first == PLConfig::FishingConfig::curFishLevel && e.second) {
                            isFilterID = true;
                            break;
                        }
                    }
                }

                bool isOffShadow = std::none_of(
                        std::begin(gPLConfig.fishing.locBong),
                        std::end(gPLConfig.fishing.locBong),
                        [](const std::pair<const int, bool> &e) {
                            return e.second;
                        }
                );


                if ((isOffShadow && gPLConfig.fishing.isLocID && !isFilterID) ||
                    (!isOffShadow && !gPLConfig.fishing.locBong[PLConfig::FishingConfig::curFishShadowLevel - 1] && !isFilterID)) {
                    DialogJoyStick::OnPress_JumpButton();
                    return;
                }
                PLConfig::FishingConfig::gFishLogger.begin(PLConfig::FishingConfig::curFishShadowLevel, PLConfig::FishingConfig::curFishLevel, PLConfig::FishingConfig::curFishZone);
                break;
            }
            case eFishingState::Hit:
            case eFishingState::BigFish_Drag:
            case eFishingState::BigFish_Stun: {
                RATE_LIMIT(50);
                DialogJoyStick::OnPress_JumpButton();
                break;
            }
            case eFishingState::Fail:
            case eFishingState::Miss:
            case eFishingState::BigFish_Miss:
            case eFishingState::BigFish_RaidFighting: {
                PLConfig::FishingConfig::gFishLogger.markFail();
                return;
            }
            default:
                break;
        }
    }



}
