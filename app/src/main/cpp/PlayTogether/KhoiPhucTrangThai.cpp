#include "PlayLog.h"
//
// Created by TEAMHMG on 13/09/2025.
//

#include "KhoiPhucTrangThai.h"
#include "Config/Config.h"
#include "../SharedPrefs/SharedPrefs.h"
#include "../base64/base64.h"
#include <json/json.hpp>
#include "SDK/ActorControl.h"
#include "SDK/LayerSystem.h"
#include "SDK/CacheUser.h"
#include "SDK/KinematicCharacterMotor.h"
#include "SDK/DialogJoyStick.h"
#include <Tools/Tools.h>

namespace KhoiPhucTrangThai {
    void SystemRestart() {
        st = State::None;
        cacheMapId = 0;
        lastMapChangeTime = 0;
        data = {};

        teleportCheckTime = 0;
        teleportTarget = {};
    }

    void Save(long hand, long veh, int map, Vector3 pos) {
        try {
            data = {map, pos, hand, veh, Item_Type::ToolItem, Item_Type::Vehicle};
            prefs.putString(OBFS("SaveDataLoad"),
                            base64_encode(nlohmann::json{
                                    {"mapID", map},
                                    {"pos",   {{"x", pos.x}, {"y", pos.y}, {"z", pos.z}}},
                                    {"hand",  hand},
                                    {"handT", (int) data.handType},
                                    {"veh",   veh},
                                    {"vehT",  (int) data.vehicleType}
                            }.dump()));
        } catch (...) {
            LOGE("KhoiPhucTrangThai::Save - Exception occurred while validating map ID");
            return;
        }
    }
    bool Load() {
        try {
            auto j = nlohmann::json::parse(base64_decode(prefs.getString(OBFS("SaveDataLoad"))));
            data.mapID = j.value("mapID", 0);
            auto p = j["pos"];
            data.pos = {p.value("x", 0.f), p.value("y", 0.f), p.value("z", 0.f)};
            data.handUID = j.value("hand", 0L);
            data.handType = (Item_Type) j.value("handT", 0);
            data.vehicleUID = j.value("veh", 0L);
            data.vehicleType = (Item_Type) j.value("vehT", 0);
            return true;
        } catch (...) {
            LOGE("KhoiPhucTrangThai::Load - Failed to parse SaveDataLoad");
            return false;
        }
    }

    void Update() {
        RATE_LIMIT(1000);
        if (!gPLConfig.general.isResetTrangThai || !ActorControl::my_Motor || !LayerSystem::get_Instance()) return;

        auto cache = CacheUser::get_Instance();
        auto actor = SystemHelper::get_Actor();
        auto equip = SystemHelper::get_Equip();
        if (!cache || !actor || !equip) return;
        auto hand = cache->get_field_object<Object *>("handEquipItem");
        if (!hand) {
            LOGE("KhoiPhucTrangThai -> handEquipItem is null");
            return;
        }

        int64_t handItemUID = hand->get_field_value<int64_t>("<ItemUID>k__BackingField");
        int64_t vehicleItemUID = actor->get_field_value<int64_t>("<RideVehicleItemUID>k__BackingField");
        int currentMapId = CacheUser::myCurrentMapID();
        if (currentMapId <= 0) {
            LOGE("KhoiPhucTrangThai -> currentMapId is invalid: %d", currentMapId);
            return;
        }

//        LOGI("KhoiPhucTrangThai::Update - State: %d, currentMapId: %d, handItemUID: %ld, vehicleItemUID: %ld, TransientPosition: (%f, %f, %f)",
//             (int) st, currentMapId, handItemUID, vehicleItemUID,
//             KinematicCharacterMotor::get_TransientPosition().x,
//             KinematicCharacterMotor::get_TransientPosition().y,
//             KinematicCharacterMotor::get_TransientPosition().z);

        switch (st) {
            case State::None:
                if (teleportCheckTime > 0) {
                    if (Tools::getSystemMilliseconds() - teleportCheckTime >= 5000) {
                        Vector3 nowPos = KinematicCharacterMotor::get_TransientPosition();
                        float dist = Vector3::Distance(nowPos, teleportTarget);

                        if (dist > 5.0f) {
                            LOGI("KhoiPhucTrangThai::Teleport verify thất bại (dist=%.2f), tele lại", dist);
                            KinematicCharacterMotor::set_TransientPosition(teleportTarget);
                            teleportCheckTime = Tools::getSystemMilliseconds(); // reset đếm lại 5s
                        } else {
                            LOGI("KhoiPhucTrangThai::Teleport verify thành công (dist=%.2f)", dist);
                            teleportCheckTime = 0;
                            teleportTarget = Vector3();
                            st = State::EquipVehicle; // hoặc State::EquipHand tuỳ logic
                        }
                    }
                } else {
                    st = Load() ? State::Teleport : State::SaveData;
                }
                break;
            case State::Teleport:
                if (!gPLConfig.fishing.isCauCa) {
                    st = State::EquipHand;
                    break;
                }
                if (data.vehicleUID > 0) {
                    if (data.mapID > 0) {
                        if (currentMapId != data.mapID) {
                            static auto timeCheckNextMap = 0;
                            if (cacheMapId != data.mapID) {
                                cacheMapId = data.mapID;
                                LOGI("KhoiPhucTrangThai::Update - NextMapPos to map ID: %d", data.mapID);
                                LayerSystem::NextMap(data.mapID);
                                timeCheckNextMap = Tools::getSystemMilliseconds();
                            } else if (Tools::getSystemMilliseconds() - timeCheckNextMap > 30000) {
                                LOGI("KhoiPhucTrangThai::Update - Timeout khi tới map ID: %d, reset lại", data.mapID);
                                cacheMapId = 0;
                                timeCheckNextMap = 0;
                            }
                        } else {
                            LOGI("KhoiPhucTrangThai::Update - đã tới map ID: %d", data.mapID);
                            if (cacheMapId > 0) lastMapChangeTime = Tools::getSystemMilliseconds();
                            cacheMapId = 0;
                            data.mapID = 0;
                        }
                    } else {
                        if (data.pos != Vector3()) {
                            if (lastMapChangeTime > 0 && Tools::getSystemMilliseconds() - lastMapChangeTime < 15000) {
                                return;
                            }

                            LOGI("KhoiPhucTrangThai::Update - Setting TransientPosition to (%f, %f, %f)",
                                 data.pos.x, data.pos.y, data.pos.z);

                            KinematicCharacterMotor::set_TransientPosition(data.pos);

                            teleportTarget = data.pos;                // lưu vị trí cần tới
                            teleportCheckTime = Tools::getSystemMilliseconds();
                            st = State::None;                         // tạm dừng, chờ verify
                            data.pos = Vector3();
                            lastMapChangeTime = 0;
                        } else {
                            st = State::EquipVehicle;
                        }
                    }
                } else if (data.handUID > 0) {
                    st = State::EquipHand;
                } else {
                    st = State::SaveData;
                }
                break;
            case State::EquipVehicle:
                if (data.vehicleUID && gPLConfig.fishing.isCauCa) {
                    if (vehicleItemUID == data.vehicleUID) {
                        if (DialogJoyStick::get_Instance()) {
                            RATE_LIMIT(3000);
                            DialogJoyStick::OnPress_JumpButton();
                            st = State::EquipHand;
                        }
                    } else {
                        void *nullVal = nullptr;
                        equip->invoke_method<void>("OnSelectItem", data.vehicleUID, data.vehicleType, nullVal);
                    }
                } else st = State::EquipHand;
                break;
            case State::EquipHand:
                if (data.handUID) {
                    void *nullVal = nullptr;
                    equip->invoke_method<void>("OnSelectItem", data.handUID, data.handType, nullVal);
                }
                st = State::SaveData;
                break;
            case State::SaveData:
                Save(handItemUID, vehicleItemUID, currentMapId, KinematicCharacterMotor::get_TransientPosition());
                break;
        }
    }
}