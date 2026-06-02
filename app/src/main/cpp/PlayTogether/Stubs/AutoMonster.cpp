//
// Created by TEAMHMG on 14/09/2025.
//

#include "AutoMonster.h"
#include "Config/Config.h"
#include "CacheUser.h"
#include "KinematicCharacterMotor.h"
#include "LayerSystem.h"
#include "TableSystem.h"
#include "DialogJoyStick.h"
#include "DialogActionButtons.h"
#include "CollectSystem.h"
#include "ActorControl.h"
#include "enum/MonsterState.h"
#include "Tools/Tools.h"
#include "PlayLog.h"

namespace Monster {

    eAutoMonster curState = eAutoMonster::None;
    List<Object*>*currentList = nullptr;
    Object*currentMonster = nullptr;
    int targetMapId = 0;

    Object*FindMonster();
    bool isValidMonster(Object*info);
    bool isTeleMonster(Object*info);

    Vector3 get_MonsterPosition(Object*info) {
        if (!info) {
            return Vector3::zero();
        }
        Object*MonsterInfo = info->get_field_object<Object*>("MonsterInfo");
        if (!MonsterInfo) {
            return Vector3::zero();
        }
        return MonsterInfo->invoke_method<Vector3>("get_Position");
    }

    void Update() {
        auto& monster = gPLConfig.monster;
        currentList = CollectSystem::_fieldMonsterInfoList();

        if (!ActorControl::my_Player) {
            return;
        }
        int mapID = CacheUser::myCurrentMapID();

        switch (curState) {
            case eAutoMonster::None:
                curState = eAutoMonster::FindMonster;
                break;
            case eAutoMonster::FindMonster:
                RATE_LIMIT(1000);
                currentMonster = FindMonster();
                if (currentMonster) {
                    curState = eAutoMonster::TeleMonster;
                } else if (monster.isTeleMapMonster) {
                    curState = eAutoMonster::NextMap;
                    LOGI("AutoDapDa: Không tìm quái vật, chuyển sang tìm kiếm bản đồ tiếp theo...");
                } else {
                    curState = eAutoMonster::None;
                    LOGI("AutoDapDa: Không tìm quái vật, đang chờ...");
                }
                break;
            case eAutoMonster::TeleMonster:
                RATE_LIMIT(1000);
                if (isTeleMonster(currentMonster)) {
                    curState = eAutoMonster::AutoAttack;
                } else {
                    curState = eAutoMonster::FindMonster;
                    currentMonster = nullptr;
                    currentList = nullptr;
                    LOGD("Reset Auto Monster: Teleport failed");
                    return;
                }
                break;
            case eAutoMonster::AutoAttack:
                RATE_LIMIT(monster.tocDoBanQuaiVat);
                if (isValidMonster(currentMonster)) {
                    Vector3 pos = get_MonsterPosition(currentMonster);
                    Vector3 myPos = KinematicCharacterMotor::get_TransientPosition();
                    if (Vector2::Distance(Vector2(pos.x, pos.z), Vector2(myPos.x, myPos.z)) > 3.0f) {
                        LOGD("Reset Auto Monster: Distance too far");
                        curState = eAutoMonster::TeleMonster;
                        currentMonster = nullptr;
                        currentList = nullptr;
                        return;
                    }
                    static int val = 0;
                    ActorControl::my_Player->invoke_method<void>("OnFieldBattleItemAction_SkipMotion", val);
                } else {
                    curState = eAutoMonster::FindMonster;
                    currentMonster = nullptr;
                    currentList = nullptr;
                }
                break;
            case eAutoMonster::NextMap: { // Chuyển sang bản đồ tiếp theo
                RATE_LIMIT(monster.delayNextMap);
                if (FindMonster() != nullptr) {
                    LOGI("AutoDapDa::Update: Đã tìm thấy quái hợp lệ, không cần chuyển sang bản đồ tiếp theo");
                    curState = eAutoMonster::None;
                    return;
                }

                auto& ds = gPLConfig.monster.DSMapMonster;
                auto it = ds.upper_bound(mapID);
                it = std::find_if(it, ds.end(), [](auto& kv) {
                    return kv.second;
                });
                if (it == ds.end()) {
                    it = std::find_if(ds.begin(), ds.end(), [](auto& kv) {
                        return kv.second;
                    });
                }
                if (it != ds.end()) {
                    targetMapId = it->first;

                    LayerSystem::NextMap(targetMapId);
                    curState = eAutoMonster::WaitingForNextMap;
                } else {
                    LOGE("AutoInsectCatcher::Update: Không tìm thấy bản đồ hợp lệ trong DSMapMonster");
                    curState = eAutoMonster::None;
                }
                break;
            }
            case eAutoMonster::WaitingForNextMap: { // Chờ chuyển sang bản đồ mới
                RATE_LIMIT(5000);
                if (mapID == targetMapId) {
                    LOGI("AutoInsectCatcher::Update: Đã chuyển sang bản đồ mới: %d", targetMapId);
                    curState = eAutoMonster::None;
                } else {
                    LOGE("AutoInsectCatcher::Update: Chuyển sang bản đồ mới không thành công, đang thử lại...");
                    curState = eAutoMonster::NextMap;
                }
                break;
            }
            default:
                curState = eAutoMonster::None;
                currentMonster = nullptr;
                currentList = nullptr;
                break;
        }
    }

    Object*FindMonster() {
        if (!currentList || currentList->get_Count() < 1) {
            return nullptr;
        }

        Object*minInfo = nullptr;
        int minHp = INT_MAX;

        for (int i = 0; i < currentList->get_Count(); i++) {
            Object*info = currentList->get_item(i);
            if (!info) {
                continue;
            }
            if (info->get_field_value<bool>("_hasReward") && gPLConfig.monster.isCollectReward) {
                info->invoke_method<void>("OnClickReward");
            }
            switch (info->get_field_value<MonsterState>("_state")) {
                case MonsterState::None:
                case MonsterState::Ready:
                case MonsterState::Appear:
                case MonsterState::Default:
                    break;
                case MonsterState::Dead:
                case MonsterState::DeadIdle:
                case MonsterState::Disappear:
                case MonsterState::Destroy:
                case MonsterState::Terminate:
                    continue;
            }
            Object*MonsterInfo = info->get_field_object<Object*>("MonsterInfo");
            if (!MonsterInfo) {
                continue;
            }

            int ResourceId = MonsterInfo->invoke_method<int>("get_ResourceId");
            if (ResourceId < 1) {
                continue;
            }
            int ObjectHp = MonsterInfo->invoke_method<int>("get_ObjectHp");
            int ObjectMaxHp = MonsterInfo->invoke_method<int>("get_ObjectMaxHp");
            if (ObjectHp <= 0 || ObjectMaxHp <= 0) {
                continue;
            } // Chỉ hiện thị quái còn sống
            if (ObjectHp > gPLConfig.monster.banQuaiVatHpDuoi) {
                continue;
            }
            if (ObjectHp < minHp) {
                minHp = ObjectHp;
                minInfo = info;
            }
        }
        return minInfo;
    }

    bool isValidMonster(Object*input) {
        if (!currentList || currentList->get_Count() < 1) {
            return false;
        }

        for (int i = 0; i < currentList->get_Count(); i++) {
            Object*info = currentList->get_item(i);
            if (!info) {
                continue;
            }
            switch (info->get_field_value<MonsterState>("_state")) {
                case MonsterState::None:
                case MonsterState::Ready:
                case MonsterState::Appear:
                case MonsterState::Default:
                    break;
                case MonsterState::Dead:
                case MonsterState::DeadIdle:
                case MonsterState::Disappear:
                case MonsterState::Destroy:
                case MonsterState::Terminate:
                    continue;
            }

            if (input == info) {
                return true;
            }
        }
        return false;
    }

    bool isTeleMonster(Object*info) {
        if (!info || !isValidMonster(info)) {
            return false;
        }
        Vector3 pos = get_MonsterPosition(info);
        Object*myTran = KinematicCharacterMotor::get_transform();
        if (!myTran) {
            return false;
        }
        Vector3 forward = myTran->invoke_method<Vector3>("get_forward").Normalize();
        float offset = 1.0f;
        Vector3 dest = Vector3(pos.x, pos.y + 5.0f, pos.z) - forward * offset;
        KinematicCharacterMotor::set_TransientPosition(dest);
        return true;
    }
}