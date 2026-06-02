//
// Created by TEAMHMG on 13/09/2025.
//

#include "ActorControl.h"
#include "CacheUser.h"
#include "Config/Config.h"
#include "SystemHelper.h"
#include "enum/eNickNameHeadUpTag.h"
#include "KhoiPhucTrangThai.h"
#include "LayerSystem.h"
#include <Tools/Tools.h>
#include <Includes/obfuscate.h>
#define LOG_TAG OBF("ATTACK_PlayTogether")
#include <Includes/Logger.h>

namespace ActorControl {
    Class *get_class() {
        return FindClass("ActorControl");
    }

    void GetListNPC() {
        volatile static int cacheMapId = 0;
        if (cacheMapId != CacheUser::myCurrentMapID()) {
            cacheMapId = CacheUser::myCurrentMapID();
            PLConfig::npcMap.clear();
            return;
        }
        if (!PLConfig::npcMap.empty()) return;
        Object *hubSystem = SystemHelper::get_Hud();
        if (!hubSystem) {
            PLConfig::npcMap.clear();
            return;
        }
        List<Object *> *_npcHeadUp3DNicknames = hubSystem->get_field_object<List<Object *> *>("_npcHeadUp3DNicknames");
        if (!_npcHeadUp3DNicknames || _npcHeadUp3DNicknames->get_Count() < 1) {
            PLConfig::npcMap.clear();
            return;
        }
        for (int i = 0; i < _npcHeadUp3DNicknames->get_Count(); i++) {
            Object *headUp = _npcHeadUp3DNicknames->get_item(i);
            if (headUp->get_field_value<eNickNameHeadUpTag>("_nickNameTag") != eNickNameHeadUpTag::NPC) continue;
            Object *nickName = headUp->get_field_object<Object *>("nickName");
            Object *cacheTransform = headUp->get_field_object<Object *>("cacheTransform");
            if (!nickName || !cacheTransform) continue;
            String *name = nickName->get_field_object<String *>("m_Text");
            if (!name) continue;
            long uid = headUp->get_field_value<long>("_uid");
            PLConfig::NPCData npcData;
            npcData.instance = headUp;
            npcData.pos = cacheTransform->invoke_method<Vector3>("get_position");
            npcData.name = name->to_string();
            npcData.uid = uid;
            PLConfig::npcMap[headUp] = npcData;
        }
    }

    Object *my_Unit = nullptr;
    Object *my_Motor = nullptr;
    Object *my_Player = nullptr;
    Object *(*old_get_Kunit)(Object *instance);

    Object *get_Kunit(Object *instance) {
        static bool inProgress = false;
        if (inProgress) {
            inProgress = false;
            return old_get_Kunit(instance);
        }
        inProgress = true;
        Object *kunit = old_get_Kunit(instance);
        if (kunit && instance) {
            if (instance->invoke_method<bool>("get_IsMyActor")) {
                my_Motor = kunit->get_field_object<Object *>("Motor");
                if (my_Motor) {
                    my_Unit = kunit;
                    my_Player = kunit->get_field_object<Object *>("actorDefaultControlPlayer");
                    static long long timeLimit = 0;
                    if (timeLimit > 0 && Tools::getSystemMilliseconds() - timeLimit < 30) {
                        return kunit;
                    }
                    timeLimit = Tools::getSystemMilliseconds();
                    Object *dialog = SystemHelper::get_Dialog();
                    if (!dialog) {
                        isGameLoading = true;
                        return kunit;
                    }
                    Object *loading = dialog->invoke_method<Object *>("get_GetLoading");
                    if (!loading) {
                        isGameLoading = true;
                        return kunit;
                    }
                    static long long choQuaMap = 0;
                    if (loading->invoke_method<bool>("GetIsLoading")) {
                        choQuaMap = Tools::getSystemMilliseconds();
                        isGameLoading = true;
                        return kunit;
                    }
                    static bool isFistLoading = true;
                    if (Tools::getSystemMilliseconds() - choQuaMap < (isFistLoading ? 10000 : 5000)) {
                        isGameLoading = true;
                        return kunit;
                    }
                    isFistLoading = false;
                    isGameLoading = false;
                    KhoiPhucTrangThai::Update();
                    LayerSystem::Update();
                    GetListNPC();
                }
            }
        }
        inProgress = false;
        return kunit;
    }
}
