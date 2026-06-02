#include "PlayLog.h"
#include "FishingSystem.h"
#include "Config/Config.h"
#include "DialogActionButtons.h"
#include "ActorControl.h"
#include "Stubs/ESPManager.h"
#include <Tools/Tools.h>
#include "System/DateTime.h"
#include "System/Nullable.h"
#include "SDK/enum/eEquipHandItemType.h"
#include "enum/Item_Type.h"
#include "CacheUser.h"
#include "NetNativeProtocol.h"
#include "NetWebProtocol.h"
#include "UnityEngine/Object.h"
#include "UnityEngine/Component.h"
#include "UnityEngine/Camera.h"
#include <Includes/obfuscate.h>
#include <imgui.h>

namespace FishingSystem {
    int MagicWaterLeft = -1;

    Class *get_class() {
        return FindClass("FishingSystem");
    }

    Object *get_Instance() {
        return SystemHelper::get_Fishing();
    }

    enum class eProductID : int {
        FisherLegend_I_1x = 90164135,
        FisherLegend_II_1x = 90164136,
        FisherLegend_III_1x = 90164137,
    };

    enum class eItemID : int {
        FisherLegend_I = 28011137,
        FisherLegend_II = 28011138,
        FisherLegend_III = 28011139
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

    int GetActiveBuffCount(int itemID) {
        auto abilitySystem = AbilitySystem_get_Self();
        if (!abilitySystem) return 0;
        if (abilitySystem->invoke_method<int>("GetCurrentFoodBuffCount") <= 0) return 0;
        List<Object *> *bufflist = abilitySystem->invoke_method<List<Object *> *>("GetMyCharacterFoodBuffList");
        if (!bufflist) return 0;
        auto now = System::DateTime::UtcNow();
        long long nowTicks = now.GetTicks();
        int count = 0;
        for (int i = 0; i < bufflist->get_Count(); i++) {
            Object *buff = bufflist->get_item(i);
            if (!buff) continue;
            if (buff->invoke_method<int>("get_ItemID") != itemID) continue;
            auto expireAt = buff->invoke_method<System::Nullable<System::DateTime>>("get_ExpireAt");
            if (expireAt.HasValue() && expireAt.Value().GetTicks() > nowTicks) count++;
        }
        return count;
    }

    uint64_t get_fishToolUID(Object *fishingSystem) {
        Object *updateFishingBait = fishingSystem->get_field_object<Object *>("UpdateFishingBait");
        if (!updateFishingBait) return 0;
        Object *target = updateFishingBait->get_field_object<Object *>("m_target");
        if (!target) return 0;
        if (target->invoke_method<eEquipHandItemType>("get_EquipItemType") != eEquipHandItemType::FishingPole) return 0;
        return target->invoke_method<uint64_t>("get_UID");
    }

    int getMaxCountMagicWater() {
        int count = 0;
        for (int i = 0; i < 3; ++i) count += gPLConfig.fishing.magicWater.levelUses[i];
        return count;
    }

    void UpdateEsp() {
        if (!gPLConfig.fishing.esp.isEnable || !gPLConfig.fishing.esp.isShowZone) return;
        RATE_LIMIT(500);
        UnityEngine::Camera *cam = UnityEngine::Camera::get_main();
        if (!cam) return;
        Array<void **> *list = UnityEngine::Component::FindObjectsOfTypeAll(UnityEngine::Component::GetType(String::Create("FisheryZoneTrigger, Assembly-CSharp")));
        if (!list) return;
        for (int i = 0; i < list->getLength(); i++) {
            void *it = list->getPointer()[i];
            if (!it) continue;
            Object *trigger = (Object *) it;
            Object *transform = trigger->invoke_method<Object *>("get_transform");
            if (!transform) continue;
            Vector3 pos = transform->invoke_method<Vector3>("get_position");
            Vector3 screenPos = cam->WorldToScreenPoint(pos);
            if (screenPos.z <= 0.f) continue;
            uint32_t zoneId = trigger->get_field_value<uint32_t>("FishingZoneID");
            std::string label = OBF("Vùng ") + std::to_string(zoneId);
            ESPManager::Add(it, pos, screenPos, label, ImVec4(0.2f, 0.8f, 1.f, 1.f));
        }
    }

    void DrawOverlay() {
        if (!gPLConfig.fishing.esp.isEnable || !gPLConfig.fishing.esp.isShowStatus) return;
        ImGui::SetNextWindowBgAlpha(0.75f);
        ImGui::SetNextWindowPos(ImVec2(10.f, 80.f), ImGuiCond_FirstUseEver);
        if (ImGui::Begin(OBF("Câu cá##fish_overlay"), nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse)) {
            ImGui::Text("%s %s", OBF("TT:"), PLConfig::FishingConfig::curStateName.c_str());
            ImGui::Text("%s %d", OBF("ID:"), PLConfig::FishingConfig::curFishLevel);
            ImGui::Text("%s %d", OBF("Bóng:"), PLConfig::FishingConfig::curFishShadowLevel);
            ImGui::Text("%s %d", OBF("Vùng:"), PLConfig::FishingConfig::curFishZone);
            ImGui::Separator();
            ImGui::Text("%s %d", OBF("Câu:"), PLConfig::FishingConfig::totalCaught);
            ImGui::Text("%s %d", OBF("Bỏ:"), PLConfig::FishingConfig::totalSkipped);
            ImGui::Text("%s %d", OBF("Hụt:"), PLConfig::FishingConfig::totalFailed);
            if (gPLConfig.fishing.magicWater.isEnable && MagicWaterLeft >= 0) {
                ImGui::Text("%s %d", OBF("Buff:"), MagicWaterLeft);
            }
        }
        ImGui::End();
    }

    void Update() {
        RATE_LIMIT(gPLConfig.fishing.delayAutoMs > 0 ? gPLConfig.fishing.delayAutoMs : 500);
        Object *instance = get_Instance();
        if (!instance || !gPLConfig.fishing.isCauCa) return;
        if (!ActorControl::my_Motor) return;
        if (instance->get_field_value<bool>("IsFishing")) {
            PLConfig::FishingConfig::curFishZone = (int) instance->get_field_value<uint32_t>("CastingFishingZoneID");
            return;
        }
        enum class eWaterState { Idle, EquipWater, UseWater, EquipRod };
        Object *abilitySystem = AbilitySystem_get_Self();
        auto &magicWater = gPLConfig.fishing.magicWater;
        static eWaterState waterState = eWaterState::Idle;
        static uint64_t lastFishToolUID = 0;
        static uint64_t pendingWaterUID = 0;
        static bool isResetToolUID = false;
        static int remainingUses[3] = {0, 0, 0};
        switch (waterState) {
            case eWaterState::Idle: {
                lastFishToolUID = get_fishToolUID(instance);
                if (!magicWater.isEnable) {
                    DialogActionButtons::OnClick();
                    break;
                }
                int buffCount = abilitySystem ? abilitySystem->invoke_method<int>("GetCurrentFoodBuffCount") : 0;
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
                    if (magicWater.levelUses[i] > 0 && buffCount < magicWater.levelUses[i] && currentActiveBuffs < getMaxCountMagicWater()) {
                        Object *waterItem = CacheUser::GetItem((int) ItemLv[i]);
                        if (waterItem) {
                            static void *nullVal = nullptr;
                            static Item_Type waterType = Item_Type::Food;
                            Object *equip = SystemHelper::get_Equip();
                            if (equip) {
                                pendingWaterUID = waterItem->invoke_method<uint64_t>("get_ItemUID");
                                if (pendingWaterUID > 0) {
                                    equip->invoke_method<void>("OnSelectItem", pendingWaterUID, waterType, nullVal);
                                    isResetToolUID = true;
                                    waterState = eWaterState::UseWater;
                                    remainingUses[i] += 1;
                                    return;
                                }
                            }
                        }
                    }
                }
                waterState = isResetToolUID ? eWaterState::EquipRod : eWaterState::Idle;
                break;
            }
            case eWaterState::UseWater: {
                RATE_LIMIT(200);
                NetNativeProtocol::SendToItemUse(pendingWaterUID);
                waterState = eWaterState::EquipWater;
                break;
            }
            case eWaterState::EquipRod: {
                RATE_LIMIT(500);
                Object *equip = SystemHelper::get_Equip();
                if (equip && lastFishToolUID > 0) {
                    static void *nullVal = nullptr;
                    static Item_Type type = Item_Type::ToolItem;
                    equip->invoke_method<void>("OnSelectItem", lastFishToolUID, type, nullVal);
                    isResetToolUID = false;
                }
                waterState = eWaterState::Idle;
                break;
            }
            default:
                waterState = eWaterState::Idle;
                break;
        }
    }
}
