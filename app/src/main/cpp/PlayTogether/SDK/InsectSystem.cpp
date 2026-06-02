#include "PlayLog.h"
//
// Created by TEAMHMG on 13/09/2025.
//

#include "InsectSystem.h"
#include "Config/Config.h"
#include "KinematicCharacterMotor.h"
#include "ActorControl.h"
#include "UnityEngine/Camera.h"
#include "TableItemImpl.h"
#include "NguiExtensions.h"
#include "Stubs/ESPManager.h"
#include "System/DateTime.h"
#include "Stubs/AutoInsect.h"
#include "kittymemory/MemoryPatch.h"
#include <Tools/Tools.h>

namespace InsectSystem { //Bắt côn trùng
    Class *get_class() {
        return FindClass("InsectSystem");
    }

    Object *get_Instance() {
        return SystemHelper::get_Insect();
    }

    float get_distance_catch() {
        Object *instance = get_Instance();
        if (!instance) return 0.f;
        Object *_catchCapsule = instance->get_field_object<Object *>("_catchCapsule");
        if (!_catchCapsule) {
            LOGE("InsectSystem::Update - _catchCapsule is null");
            return 0.f;
        }
        Vector3 center = _catchCapsule->invoke_method<Vector3>("get_Center");
        Vector3 myPos = KinematicCharacterMotor::get_TransientPosition();
        return Vector3::Distance(center, myPos);
    }
    std::string getFilterText(int value) {
        const char *filters[] = {
                "Trằng", "Xanh lá", "Xanh nước", "Tím (siêu hiếm)", "Ngũ Sắc"
        };
        const int count = sizeof(filters) / sizeof(filters[0]);
        value = std::max(1, std::min(value, count));
        return std::string(filters[value - 1]);
    }
    void Update() {
        Object *instance = get_Instance();
        if (!instance) return;
        auto &insect = gPLConfig.insect;


        static MemoryPatch DetectProcess = MemoryPatch::createWithHex((uintptr_t) FindClass("InsectController")->find_method("DetectProcess")->methodPointer, HEX_NOP);
        static MemoryPatch SenseCheck = MemoryPatch::createWithHex((uintptr_t) FindClass("InsectController")->find_method("SenseCheck")->methodPointer, HEX_NOP);
        if (SenseCheck.isValid() && DetectProcess.isValid()) {
            if (insect.isFreezeCT) {
                DetectProcess.Modify();
                SenseCheck.Modify();
            } else {
                DetectProcess.Restore();
                SenseCheck.Restore();
            }
        }

        if (!(insect.isAutoBatBo || insect.esp.isEnable)) return;
        if (!ActorControl::my_Motor) return;

        UnityEngine::Camera *camera = UnityEngine::Camera::get_main();
        if (!camera || !camera->isValid()) {
            return;
        }


        List<Object *> *_liveInsectList = instance->get_field_object<List<Object *> *>("_liveInsectList");
        if (!_liveInsectList) return;
        if (insect.esp.isEnable) {
            for (int i = 0; i < _liveInsectList->get_Count(); i++) {
                Object *insects = _liveInsectList->get_item(i);
                if (!insects) continue;
                Object *control = insects->get_field_object<Object *>("control");
                if (!control) continue;
                int id = control->invoke_method<int>("get_SubjectID");
                if (id < 1) continue;

                int grade = TableItemImpl::GetGrade(id);
                if (grade < 1) {
                    continue;
                }
                bool isShow = false;
                for (int j = 0; j < 5; j++) {
                    if (grade == j + 1) {
                        if (insect.esp.isShowGrade[j]) {
                            isShow = true;
                        }
                    }
                }
                if (!isShow) {
                    continue;
                }

                if (!control->get_field_value<bool>("IsVisible")) continue;


                std::string name = NguiExtensions::GetNameIDText(id);
                if (name.empty()) {
                    name += "HỘP QUÀ/GÓI THẺ";
                }

                name += "\nNền: " + getFilterText(grade);

                {// Kiểm tra bọ an toàn
                    if (name.find("(") != std::string::npos) { // Phát hiện có bọ tên "곤충(Insect) 이름" trong góc lag
                        continue;
                    }
                    int _zoneID = control->get_field_value<int>("_zoneID");
                    if (_zoneID < 1001 || _zoneID > 9000) continue; // Phát hiện có bọ có _zoneID 9999 trong góc lag
                }

                Object *transform = control->invoke_method<Object *>("get_transform");
                if (!transform) continue;

                Vector3 pos = transform->invoke_method<Vector3>("get_position");
                Vector3 screenPos = camera->WorldToScreenPoint(pos);

                ESPManager::Add(insects, (insect.esp.isTeleportButton ? pos : Vector3()), screenPos, (insect.esp.isShowName ? name : ""));
            }
        }

        if (insect.isAutoBatBo) {
            RATE_LIMIT(500);
            System::DateTime _nextRequestTime = instance->get_field_value<System::DateTime>("_nextRequestTime");
            System::DateTime now = System::DateTime::Now();
            System::DateTime localNow = now.ToLocalTime();
            System::DateTime localNext = _nextRequestTime.ToLocalTime();

            long long ticksDiff = localNext.GetTicks() - localNow.GetTicks();
            long long totalSeconds = ticksDiff / 10000000; // 1s = 10^7 ticks
            if (totalSeconds < 0) totalSeconds = 0;

            int minutes = static_cast<int>(totalSeconds / 60);
            int seconds = static_cast<int>(totalSeconds % 60);

            char buffer[32];
            if (minutes > 0)
                snprintf(buffer, sizeof(buffer), "%dm %ds", minutes, seconds);
            else
                snprintf(buffer, sizeof(buffer), "%ds", seconds);

            PLConfig::InsectConfig::dateResetInsect = buffer;


            InsectSys::Update(_liveInsectList);
        }


    }
}