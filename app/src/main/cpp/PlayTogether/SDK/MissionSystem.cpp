#include "PlayLog.h"
//
// Created by TEAMHMG on 13/09/2025.
//

#include "MissionSystem.h"
#include "Config/Config.h"
#include "NetNativeProtocol.h"
#include <Tools/Tools.h>

namespace MissionSystem {
    Class *get_class() {
        return FindClass("MissionSystem");
    }
    Object *get_Instance() {
        return SystemHelper::get_Mission();
    }

    std::map<void *, Achievement> achievementList;

    void Update() { //Nhận thành tích
        RATE_LIMIT(5000);
        Object *instance = get_Instance();
        if (!instance) return;

        //Cho nhân vật không ngủ
        instance->set_field_value("SleepElapsedT", std::numeric_limits<float>::max());

        //Tự đông nhận thành tích
        if (!gPLConfig.general.isNhanThanhTich) {
            return;
        }
        List<Object *> *completeAchievementList = instance->get_field_object<List<Object *> *>("completeAchievementList");
        if (completeAchievementList) {
            for (int i = 0; i < completeAchievementList->get_Count(); i++) {
                Object *achievement = completeAchievementList->get_item(i);
                if (achievement) {
                    int AchievementId = achievement->get_field_value<int>("<AchievementId>k__BackingField");
                    int Step = achievement->get_field_value<int>("<Step>k__BackingField");
                    achievementList.insert({achievement, {AchievementId, Step}});
                }
            }
        }
        for (auto it = achievementList.begin(); it != achievementList.end(); it++) {
            Achievement &achievement = it->second;
            if (achievement.isComplete) continue;
            int AchievementId = achievement.AchievementId;
            int Step = achievement.Step;
            achievement.isComplete = true;
            LOGI("AchievementId: %d, Step: %d", AchievementId, Step);
            NetNativeProtocol::SendToAchievementReward(AchievementId, Step);
        }
    }

}