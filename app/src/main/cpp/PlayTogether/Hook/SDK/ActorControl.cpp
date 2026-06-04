//
// Created by TEAMHMG on 13/09/2025.
//

#include "ActorControl.h"
#include "../../Config/Config.h"
#include "SystemHelper.h"
#include "../AutoFishing/AutoFishing.h"
#include <Tools/Tools.h>
#define LOGGER_TAG "ATTACK_PlayTogether"
#include <Includes/Logger.h>

namespace ActorControl {
    Class *get_class() {
        return FindClass(OBF("ActorControl"));
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
            if (instance->invoke_method<bool>(OBF("get_IsMyActor"))) {
                my_Motor = kunit->get_field_object<Object *>(OBF("Motor"));
                if (my_Motor) {
                    my_Unit = kunit;
                    my_Player = kunit->get_field_object<Object *>(OBF("actorDefaultControlPlayer"));
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
                    Object *loading = dialog->invoke_method<Object *>(OBF("get_GetLoading"));
                    if (!loading) {
                        isGameLoading = true;
                        inProgress = false;
                        return kunit;
                    }
                    static long long loadingSettledAt = 0;
                    static bool isFirstLoading = true;
                    if (loading->invoke_method<bool>(OBF("GetIsLoading"))) {
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
                    if (gPLConfig.fishing.enabled || gPLConfig.fishing.autoEquipBait || gPLConfig.fishing.autoCraftBait) {
                        AutoFishing::Update();
                    }
                    AutoFishing::UpdatePickerFromGameThread();
                }
            }
        }
        inProgress = false;
        return kunit;
    }
}
