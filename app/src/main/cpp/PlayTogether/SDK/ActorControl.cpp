//
// Created by TEAMHMG on 13/09/2025.
//

#include "ActorControl.h"
#include "Config/Config.h"
#include "SystemHelper.h"
#include "../AutoFishing.h"
#include "../UI/OverlaySnapshot.h"
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
                        inProgress = false;
                        return kunit;
                    }
                    timeLimit = Tools::getSystemMilliseconds();
                    Object *dialog = SystemHelper::get_Dialog();
                    if (!dialog) {
                        isGameLoading = true;
                        inProgress = false;
                        return kunit;
                    }
                    Object *loading = dialog->invoke_method<Object *>("get_GetLoading");
                    if (!loading) {
                        isGameLoading = true;
                        inProgress = false;
                        return kunit;
                    }
                    static long long loadingSettledAt = 0;
                    static bool isFirstLoading = true;
                    if (loading->invoke_method<bool>("GetIsLoading")) {
                        loadingSettledAt = 0;
                        isGameLoading = true;
                        inProgress = false;
                        return kunit;
                    }
                    if (loadingSettledAt == 0) {
                        loadingSettledAt = Tools::getSystemMilliseconds();
                    }
                    const long long settleMs = isFirstLoading ? 10000 : 5000;
                    if (Tools::getSystemMilliseconds() - loadingSettledAt < settleMs) {
                        isGameLoading = true;
                        inProgress = false;
                        return kunit;
                    }
                    isFirstLoading = false;
                    isGameLoading = false;
                    if (gPLConfig.fishing.enabled) {
                        AutoFishing::Update();
                    }
                    OverlaySnapshot::UpdateFromGameThread();
                }
            }
        }
        inProgress = false;
        return kunit;
    }
}
