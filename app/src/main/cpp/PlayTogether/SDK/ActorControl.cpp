//
// Created by TEAMHMG on 13/09/2025.
//

#include "ActorControl.h"
#include "Config/Config.h"
#include "SystemHelper.h"
#include "../AutoFishing.h"
#include <Tools/Tools.h>
#include <Includes/obfuscate.h>
#define LOG_TAG OBF("ATTACK_PlayTogether")
#include <Includes/Logger.h>

namespace ActorControl {
    Class *get_class() {
        return FindClass("ActorControl");
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
                    AutoFishing::Update();
                }
            }
        }
        inProgress = false;
        return kunit;
    }
}
