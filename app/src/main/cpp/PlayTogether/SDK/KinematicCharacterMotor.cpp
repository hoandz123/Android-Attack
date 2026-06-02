//
// Created by TEAMHMG on 13/09/2025.
//

#include "KinematicCharacterMotor.h"
#include "Stubs/AutoInsect.h"
#include "Config/Config.h"
#include "ActorControl.h"

namespace KinematicCharacterMotor { //Dịch chuyển
    Class *get_class() {
        return FindClass("KinematicCharacterController.KinematicCharacterMotor");
    }

    Object *get_transform() {
        if (!ActorControl::my_Motor) {
            return nullptr;
        }
        return ActorControl::my_Motor->invoke_method<Object *>("get_transform");
    }

    Vector3 get_TransientPosition() {
        if (!ActorControl::my_Motor) {
            return Vector3();
        }
        return ActorControl::my_Motor->invoke_method<Vector3>("get_TransientPosition");
    }
    void set_TransientPosition(Vector3 position) {
        if (!ActorControl::my_Motor) {
            return;
        }
        ActorControl::my_Motor->invoke_method<void>("set_TransientPosition", position);
    }

    void (*old_UpdatePhase1)(Object *instance, float deltaTime);
    void UpdatePhase1(Object *instance, float deltaTime) {
        if (gPLConfig.insect.isBatBoTrenTroi) {
            if (!InsectSys::isStopDiTrenKhong && !isGameLoading) {
                return;
            }
        }
        old_UpdatePhase1(instance, deltaTime);
    }
}
