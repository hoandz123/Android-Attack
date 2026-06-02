//
// Created by PC on 08/10/2025.
//

#include "MiniGameKMGUnit.h"
#include "Tools/Tools.h"
#include "enum/KnockOutMiniGame_Type.h"
#include "enum/eBlockState.h"
#include "KinematicCharacterMotor.h"
#include "enum/KMG_Racing_Type.h"
#include "enum/GameBattleState.h"
#include "enum/KMG_Game_Type.h"
#include "Config/Config.h"
#include "UnityEngine/Object.h"
#include "DialogJoyStick.h"
#include "enum/eBubbleTipType.h"
#include "PlayLog.h"

namespace MiniGameKMGUnit {
    Class *get_class() {
        return FindClass("PlayTogether.MiniGame.MiniGameKMGUnit");
    }


    void (*old_Update)(...);

    void Update(Object *instance) {
        old_Update(instance);
        if (!instance) return;
        RATE_LIMIT(60);
        auto &Party = gPLConfig.miniGame.Party;
        if (!Party.isEnable) return;

        GameBattleState currentBattleState = instance->get_field_value<GameBattleState>("currentBattleState");
        bool isCurrentSuccess = instance->get_field_value<bool>("isCurrentSuccess");

        Object *_myActorControl = instance->get_field_object<Object *>("_myActorControl");
        if (!_myActorControl) {
            LOGE("_myActorControl is null");
            return;
        }

        if (currentBattleState != GameBattleState::BattleStateStart || isCurrentSuccess) {
            gPLConfig.insect.isBatBoTrenTroi = false;
            return;
        }
        KnockOutMiniGame_Type CurrentKMGType = instance->get_field_value<KnockOutMiniGame_Type>("<CurrentKMGType>k__BackingField");
        KMG_Game_Type CurrentKMGGameType = instance->get_field_value<KMG_Game_Type>("<CurrentKMGGameType>k__BackingField");
//        KMG_Racing_Type CurrentKMGRacingType = instance->get_field_value<KMG_Racing_Type>("<CurrentKMGRacingType>k__BackingField");
        int KMGTableID = instance->get_field_value<int>("<KMGTableID>k__BackingField");
//        static KnockOutMiniGame_Type cacheKMGType = KnockOutMiniGame_Type::None;
//        if (cacheKMGType != CurrentKMGType) {
//            cacheKMGType = CurrentKMGType;
            LOGD("class: %s", instance->get_class()->get_name().c_str());
            LOGD("MiniGameKMGUnit Mini game type changed: %d", (int) CurrentKMGType);
//        }
        switch (CurrentKMGType) {
            case KnockOutMiniGame_Type::FloorRandom: { //MiniGameKMGFloorRandom
                if (!gPLConfig.insect.isBatBoTrenTroi) {
                    gPLConfig.insect.isBatBoTrenTroi = true;
                }
                break;
            }
            case KnockOutMiniGame_Type::FloorPaint: { //MiniGameKMGFloorPaint
                RATE_LIMIT(1000);
                Vector3 myPos = KinematicCharacterMotor::get_TransientPosition();
                KinematicCharacterMotor::set_TransientPosition(Vector3(myPos.x, myPos.y - 15.0f, myPos.z));
                break;
            }
            case KnockOutMiniGame_Type::Racing: { //MiniGameKMGTwistRoad
                //Goal_Model
                //Finish_line
                //Stage_Goal


                if (KMGTableID == 120032) { //Trèo lên tòa nhà
                    RATE_LIMIT(Party.delayNextPoint);
                    KinematicCharacterMotor::set_TransientPosition(Vector3(0.98583984375,53.531005859375,-6.89129638671875));
                    return;
                }
                if (KMGTableID == 120015) { //Mê cung lọt khe
                    RATE_LIMIT(Party.delayNextPoint);
                    KinematicCharacterMotor::set_TransientPosition(Vector3(0.01116943359375,0.0003662109375,0.26055908203125));
                    return;
                }
                if (KMGTableID == 120028) { //Map Băng
                    RATE_LIMIT(Party.delayNextPoint);
                    KinematicCharacterMotor::set_TransientPosition(Vector3(-18.7320556640625,13.0355224609375,-78.353759765625));
                    return;
                }

                if (KMGTableID == 120033) { //Máp UFO
                    RATE_LIMIT(Party.delayNextPoint);
                    KinematicCharacterMotor::set_TransientPosition(Vector3(-5.031982421875,13.430038452148438,-98.392333984375));
                    return;
                }

                if (KMGTableID == 120018) { //Máp kính
                    RATE_LIMIT(Party.delayNextPoint);
                    KinematicCharacterMotor::set_TransientPosition(Vector3(-0.499634, 23.246078, 26.613281));
                    return;
                }
                if (KMGTableID != 120042) {
                    //Vượt đường tàu
                    Object *StageManager = instance->get_field_object<Object *>("StageManager");
                    if (StageManager) {
                        RATE_LIMIT(Party.delayNextPoint);
                        Object *GoalStageRoot = StageManager->get_field_object<Object *>("GoalStageRoot");
                        if (GoalStageRoot) {
                            LOGI("StageManager Teleport to Racing goal");
                            Vector3 pos = GoalStageRoot->invoke_method<Vector3>("get_position");
                            if (pos != Vector3(0, 0, 0)) {
                                pos.z += 5.0f;
                                KinematicCharacterMotor::set_TransientPosition(pos);
                                return;
                            }
                        } else {
                            LOGE("GoalStageRoot is null");
                        }
                    } else {
                        LOGE("StageManager is null");
                    }

                    //goalCharSettingTrigger
                    Object *goalCharSettingTrigger = instance->get_field_object<Object *>("goalCharSettingTrigger");
                    if (goalCharSettingTrigger) {
                        RATE_LIMIT(Party.delayNextPoint);
                        Object *transformTarget = goalCharSettingTrigger->invoke_method<Object *>("get_transform");
                        if (transformTarget) {
                            LOGI("goalCharSettingTrigger Teleport to Racing goal");
                            Vector3 pos = transformTarget->invoke_method<Vector3>("get_position");
                            if (KMGTableID == 120004) pos = Vector3(1.4434814453125,-65.27899169921875,92.5614013671875);
                            if (pos != Vector3(0, 0, 0)) {
                                pos.y += 5.0f;
                                pos.z += 5.0f;
                                KinematicCharacterMotor::set_TransientPosition(pos);
                                return;
                            }
                        }
                    }

                    //Vượt mê cung
                    Object *EndPointGo = instance->get_field_object<Object *>("EndPointGo");
                    if (EndPointGo) {
                        RATE_LIMIT(Party.delayNextPoint);
                        Object *transformTarget = EndPointGo->invoke_method<Object *>("get_transform");
                        if (transformTarget) {
                            LOGI("EndPointGo Teleport to Racing goal");
                            Vector3 pos = transformTarget->invoke_method<Vector3>("get_position");
                            if (pos != Vector3(0, 0, 0)) {
                                pos.y += 5.0f;
                                KinematicCharacterMotor::set_TransientPosition(pos);
                                return;
                            }
                        }
                    }

                    //endKeppRoot
                    Object *endKeppRoot = instance->get_field_object<Object *>("endKeppRoot");
                    if (endKeppRoot) {
                        RATE_LIMIT(Party.delayNextPoint);
                        Object *transformTarget = endKeppRoot->invoke_method<Object *>("get_transform");
                        if (transformTarget) {
                            LOGI("endKeppRoot Teleport to Racing goal");
                            Vector3 pos = transformTarget->invoke_method<Vector3>("get_position");
                            if (pos != Vector3(0, 0, 0)) {
                                if (KMGTableID == 120006) {
                                    pos.y += 1.0f;
                                    pos.z += 1.0f;
                                }
                                KinematicCharacterMotor::set_TransientPosition(pos);
                                return;
                            } else {
                                Array<void **> *list = UnityEngine::Object::FindObjectsOfTypeAll(FindClass("UnityEngine.Transform")->get_type()->get_object());
                                for (int i = 0; i < list->max_length; i++) {
                                    System::Object *tf = (System::Object *) list->getPointer()[i];
                                    if (!tf) continue;
                                    if (tf->ToString().find("Goal_Model") == std::string::npos &&
                                        tf->ToString().find("Finish_line") == std::string::npos &&
                                        tf->ToString().find("Stage_Goal") == std::string::npos) {
                                        LOGI("Skip Transform name: %s", tf->ToString().c_str());
                                        continue;
                                    }
                                    Vector3 p = tf->invoke_method<Vector3>("get_position");
                                    if (p != Vector3(0, 0, 0)) {
                                        LOGI("Found Transform name: %s", tf->ToString().c_str());
                                        LOGI("Teleport to Racing goal: %f, %f, %f", p.x, p.y, p.z);
                                        p.y += 5.0f;
                                        KinematicCharacterMotor::set_TransientPosition(p);
                                        return;
                                    }
                                }
                            }
//                        Array<void **> * list = UnityEngine::Object::FindObjectsOfTypeAll(FindClass("UnityEngine.Transform")->get_type()->get_object());
//                        for (int i = 0; i < list->max_length; i++) {
//                            System::Object* tf = (System::Object*)list->getPointer()[i];
//                            if (!tf) continue;
//                            Vector3 p = tf->invoke_method<Vector3>("get_position");
//                            if (pos == p) {
//                                LOGI("Found Transform name: %s", tf->ToString().c_str());
//                                continue;
//                            }
//                        }
                            return;
                        }

                    }
                }
                switch (CurrentKMGGameType) {
                    case KMG_Game_Type::Solo: {
                        LOGD("CurrentKMGGameType: Solo");
                        if (KMGTableID == 120042) {
                            RATE_LIMIT(3000);
                        }
                        if (!gPLConfig.insect.isBatBoTrenTroi) {
                            gPLConfig.insect.isBatBoTrenTroi = true;
                            Vector3 myPos = KinematicCharacterMotor::get_TransientPosition();
                            KinematicCharacterMotor::set_TransientPosition(Vector3(myPos.x, myPos.y + 15.0f, myPos.z));
                            return;
                        }
                        break;
                    }
                    case KMG_Game_Type::Team: {
                        LOGD("CurrentKMGGameType: Team");
                        break;
                    }
                    case KMG_Game_Type::Asymmetric: {
                        LOGD("CurrentKMGGameType: Asymmetric");
                        break;
                    }
                    default:
                        break;
                }


                break;
            }
            case KnockOutMiniGame_Type::StealCrown: {

                break;
            }
            case KnockOutMiniGame_Type::PassBomb: { //MiniGameKMGPassBomb - Chuyền bom

                Object *actorBase = _myActorControl->get_field_object<Object *>("actorBase");
                if (!actorBase) {
                    LOGE("actorBase is null");
                    return;
                }
                if (!gPLConfig.insect.isBatBoTrenTroi) {
                    gPLConfig.insect.isBatBoTrenTroi = true;
                    Vector3 myPos = KinematicCharacterMotor::get_TransientPosition();
                    KinematicCharacterMotor::set_TransientPosition(Vector3(myPos.x, myPos.y + 15.0f, myPos.z));
                }
                long mySUID = actorBase->get_field_value<long>("<SUID>k__BackingField");

                List<Object *> *passBombCounters = instance->get_field_object<List<Object *> *>("passBombCounters");
                if (!passBombCounters) {
                    LOGE("passBombCounters is null");
                    return;
                }
                bool isFound = false;
                for (int i = 0; i < passBombCounters->get_Count(); i++) {
                    Object *counter = passBombCounters->get_item(i);
                    if (!counter) continue;
                    long SelectUID = counter->get_field_value<long>("<SelectUID>k__BackingField");
                    if (SelectUID > 0 && SelectUID == mySUID) {
                        LOGD("Đang giữ bom");
                        isFound = true;
                    }
                }
                if (isFound) for (int i = 0; i < passBombCounters->get_Count(); i++) {
                    Object *counter = passBombCounters->get_item(i);
                    if (!counter) continue;
                    long SelectUID = counter->get_field_value<long>("<SelectUID>k__BackingField");
                    if (SelectUID > 0 && SelectUID != mySUID) {
                        LOGD("Đang giữ bom đổi cho UID: %ld", SelectUID);
                        Object* attachParent = counter->get_field_object<Object*>("attachParent");
                        if (attachParent) {
                            KinematicCharacterMotor::set_TransientPosition(attachParent->invoke_method<Vector3>("get_position"));
                        }
                    }
                }

                break;
            }
            case KnockOutMiniGame_Type::Duel: { //MiniGameKMGDuel - Đấu tay đôi
                LOGD("KnockOutMiniGame_Type::Duel");
                if (instance->get_field_value<bool>("<IsCanAttack>k__BackingField")) {
                    LOGD("IsCanAttack true, auto jump");
                    Object* _actorDuelPlayer = instance->get_field_object<Object*>("_actorDuelPlayer");
                    if (_actorDuelPlayer) {
                        _actorDuelPlayer->invoke_method<void>("OnAction");
                    }
                }
                break;
            }
            case KnockOutMiniGame_Type::GreenRedLight: { //Xanh Đỏ
                RATE_LIMIT(10000);
                Object *GoRobotGirlFace = instance->get_field_object<Object *>("GoRobotGirlFace");
                if (!GoRobotGirlFace) {
                    LOGE("GoRobotGirlFace is null");
                    return;
                }
                Object *transformFace = GoRobotGirlFace->invoke_method<Object *>("get_transform");
                if (!transformFace) {
                    LOGE("transformFace is null");
                    return;
                }
                LOGI("GoRobotGirlFace Teleport to Face");
                Vector3 posFace = transformFace->invoke_method<Vector3>("get_position");
                posFace.z += 5.0f;
                KinematicCharacterMotor::set_TransientPosition(posFace);
                break;
            }
            case KnockOutMiniGame_Type::RealBoard: { //MiniGameKMGRealBoard
                instance->invoke_method<void>("WarpToGoalGround");
//                if (!gPLConfig.insect.isBatBoTrenTroi) {
//                    gPLConfig.insect.isBatBoTrenTroi = true;
//                    Vector3 myPos = KinematicCharacterMotor::get_TransientPosition();
//                    KinematicCharacterMotor::set_TransientPosition(Vector3(myPos.x, myPos.y + 5.0f, myPos.z));
//                }
                break;
            }
            case KnockOutMiniGame_Type::Mingle: { //MiniGameKMGMingle

                break;
            }
            default:
                break;
        }
    }


    void init() {
        Tools::Hook(get_class()->find_method("Update")->methodPointer, (void *) Update, (void **) &old_Update);
    }
}

