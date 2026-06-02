#include "PlayLog.h"
#include "ActorDefaultControlPlayer.h"
#include "TableSystem.h"
#include "Config/Config.h"
#include "ActorControl.h"
#include "enum/eFishingState.h"
#include "DialogJoyStick.h"
#include "FrameWork.h"
#include "KhoiPhucTrangThai.h"
#include "TableFishingDifficultyImpl.h"
#include "enum/eTableType.h"
#include <Tools/Tools.h>
#include <map>
#include <cstdint>
#include "UnityEngine/Object.h"
#include "UnityEngine/Component.h"

namespace ActorDefaultControlPlayer {

namespace {

const char *FishingStateName(eFishingState state) {
    switch (state) {
        case eFishingState::None: return "None";
        case eFishingState::Casting: return "Quăng cần";
        case eFishingState::Search: return "Tìm cá";
        case eFishingState::SearchResult: return "Phát hiện";
        case eFishingState::Idle: return "Chờ cắn";
        case eFishingState::Hit: return "Cắn câu";
        case eFishingState::Fighting: return "Kéo cá";
        case eFishingState::Catch: return "Bắt được";
        case eFishingState::Fail: return "Thất bại";
        case eFishingState::Boast: return "Khoe cá";
        case eFishingState::Finish: return "Xong";
        case eFishingState::CastingFail: return "Quăng hụt";
        case eFishingState::Miss: return "Trượt";
        case eFishingState::BigFish_RaidEnter: return "Raid cá lớn";
        case eFishingState::BigFish_Begin: return "Cá lớn";
        case eFishingState::BigFish_Pumpin: return "Pump";
        case eFishingState::BigFish_Drag: return "Kéo cá lớn";
        case eFishingState::BigFish_Tug: return "Giật cần";
        case eFishingState::BigFish_Fighting: return "Chiến cá lớn";
        case eFishingState::BigFish_Catch: return "Bắt cá lớn";
        case eFishingState::BigFish_Miss: return "Mất cá lớn";
        case eFishingState::BigFish_RaidFighting: return "Raid fight";
        case eFishingState::BigFish_StunBegin: return "Choáng";
        case eFishingState::BigFish_Stun: return "Choáng cá";
        case eFishingState::BigFish_StunRecovery: return "Hồi choáng";
        default: return "Unknown";
    }
}

void AutoJump() {
    DialogJoyStick::OnPress_JumpButton();
}

}

    Class *get_class() {
        return FindClass("ActorDefaultControlPlayer");
    }

    std::map<void*, int> originalValues;

    void SetNewZone(int zoneId) {
        Array<void **> *list = UnityEngine::Component::FindObjectsOfTypeAll(UnityEngine::Component::GetType(String::Create("FisheryZoneTrigger, Assembly-CSharp")));
        if (!list) return;
        for (int i = 0; i < list->getLength(); i++) {
            void *it = list->getPointer()[i];
            if (!it) continue;
            uintptr_t offset = (uintptr_t) it + IL2Cpp::Il2CppGetFieldOffset("FisheryZoneTrigger", "FishingZoneID");
            int *valuePtr = (int *) offset;
            if (originalValues.find(it) == originalValues.end()) originalValues[it] = *valuePtr;
            *valuePtr = zoneId;
        }
    }

    void RestoreZone() {
        Array<void **> *list = UnityEngine::Component::FindObjectsOfTypeAll(UnityEngine::Component::GetType(String::Create("FisheryZoneTrigger, Assembly-CSharp")));
        if (!list) return;
        for (int i = 0; i < list->getLength(); i++) {
            void *it = list->getPointer()[i];
            if (!it) continue;
            uintptr_t offset = (uintptr_t) it + IL2Cpp::Il2CppGetFieldOffset("FisheryZoneTrigger", "FishingZoneID");
            int *valuePtr = (int *) offset;
            auto it2 = originalValues.find(it);
            if (it2 != originalValues.end()) *valuePtr = it2->second;
        }
    }

    static uint32_t EffectiveFishingZoneId() {
        if (gPLConfig.fishing.isFakeVR) return 503u;
        if (gPLConfig.fishing.isFishZone && gPLConfig.fishing.fishZone > 0) return (uint32_t) gPLConfig.fishing.fishZone;
        return 0u;
    }

    static Object *FishingArea_FindByZoneId(uint32_t zoneId) {
        Object *table = TableSystem::GetTableUnit<Object *>(
                FindClass("PlayTogether.Table.eTableType")->get_enum_value<eTableType>("FishingArea"));
        if (!table) return nullptr;
        return table->invoke_method<Object *>("GetTableData", zoneId);
    }

    void (*old_SendToFishingCasting)(void *, void *);

    void Hook_SendToFishingCasting(void *thiz, void *fishingZoneList) {
        if (fishingZoneList) {
            List<uint32_t> *list = (List<uint32_t> *) fishingZoneList;
            const int n = list->get_Count();
            const uint32_t replaceZone = EffectiveFishingZoneId();
            for (int i = 0; i < n; i++) {
                uint32_t v = list->get_item(i);
                if (v > 0u && replaceZone > 0u) list->set_item(i, replaceZone);
            }
        }
        old_SendToFishingCasting(thiz, fishingZoneList);
    }

    void (*old_DialogZoneTitle_SetTitle)(void *, uint32_t);

    void Hook_DialogZoneTitle_SetTitle(void *thiz, uint32_t titleID) {
        uint32_t id = titleID;
        const unsigned z = EffectiveFishingZoneId();
        if (z > 0u) {
            Object *row = FishingArea_FindByZoneId((uint32_t) z);
            if (row) {
                uint32_t textId = row->invoke_method<uint32_t>("get_FishingZoneText");
                if (textId > 0u) id = textId;
            }
        }
        old_DialogZoneTitle_SetTitle(thiz, id);
    }

    int GetFishShadowLevel(int level) {
        Object *difficulty = TableFishingDifficultyImpl::GetTableData(level);
        if (!difficulty) return 0;
        String *AssetName = difficulty->get_field_object<String *>("<AssetName>k__BackingField");
        if (!AssetName) return 0;
        std::string fishShadow = AssetName->to_string();
        std::string keywords[] = {
                "fish_s_shadow", "fish_m_shadow", "fish_l_shadow",
                "fish_xl_shadow", "fish_xxl_shadow", "fish_xxxl_shadow",
                "fish_4xl_shadow"
        };
        for (int i = 0; i < 7; i++) {
            if (fishShadow.find(keywords[i]) != std::string::npos) return i + 1;
        }
        return 0;
    }

    void Update() {
        RATE_LIMIT(gPLConfig.fishing.delayAutoMs > 0 ? gPLConfig.fishing.delayAutoMs : 60);
        Object *instance = ActorControl::my_Player;
        if (!instance || !gPLConfig.fishing.isCauCa) return;
        eFishingState state = instance->invoke_method<eFishingState>("get_FishingState");
        PLConfig::FishingConfig::curStateName = FishingStateName(state);
        switch (state) {
            case eFishingState::None:
                break;
            case eFishingState::Idle: {
                PLConfig::FishingConfig::curFishLevel = instance->invoke_method<int>("get_FishLevel");
                if (PLConfig::FishingConfig::curFishLevel <= 0) {
                    AutoJump();
                    return;
                }
                PLConfig::FishingConfig::curFishShadowLevel = GetFishShadowLevel(PLConfig::FishingConfig::curFishLevel);
                if (PLConfig::FishingConfig::curFishShadowLevel <= 0) {
                    AutoJump();
                    return;
                }
                bool isFilterID = false;
                if (gPLConfig.fishing.isLocID) {
                    for (auto &e : gPLConfig.fishing.IDLocCa) {
                        if (e.first == PLConfig::FishingConfig::curFishLevel && e.second) {
                            isFilterID = true;
                            break;
                        }
                    }
                }
                bool isOffShadow = std::none_of(
                        gPLConfig.fishing.locBong.begin(),
                        gPLConfig.fishing.locBong.end(),
                        [](const std::pair<const int, bool> &e) { return e.second; });
                bool shouldSkip = (isOffShadow && gPLConfig.fishing.isLocID && !isFilterID)
                    || (!isOffShadow && !gPLConfig.fishing.locBong[PLConfig::FishingConfig::curFishShadowLevel - 1] && !isFilterID);
                if (shouldSkip) {
                    PLConfig::FishingConfig::totalSkipped++;
                    AutoJump();
                    return;
                }
                PLConfig::FishingConfig::gFishLogger.begin(
                    PLConfig::FishingConfig::curFishShadowLevel,
                    PLConfig::FishingConfig::curFishLevel,
                    PLConfig::FishingConfig::curFishZone);
                break;
            }
            case eFishingState::Hit:
            case eFishingState::Fighting:
            case eFishingState::BigFish_Drag:
            case eFishingState::BigFish_Stun:
            case eFishingState::BigFish_Pumpin:
            case eFishingState::BigFish_Tug:
            case eFishingState::BigFish_Fighting:
            case eFishingState::BigFish_StunBegin:
            case eFishingState::BigFish_StunRecovery:
            case eFishingState::Catch:
            case eFishingState::Finish:
            case eFishingState::Boast:
            case eFishingState::BigFish_Catch: {
                RATE_LIMIT(50);
                AutoJump();
                break;
            }
            case eFishingState::Fail:
            case eFishingState::Miss:
            case eFishingState::CastingFail:
            case eFishingState::BigFish_Miss:
            case eFishingState::BigFish_RaidFighting: {
                PLConfig::FishingConfig::totalFailed++;
                PLConfig::FishingConfig::gFishLogger.markFail();
                break;
            }
            default:
                break;
        }
    }
}
