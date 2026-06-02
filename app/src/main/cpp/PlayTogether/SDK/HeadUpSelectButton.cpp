#include "PlayLog.h"
//
// Created by TEAMHMG on 13/09/2025.
//

#include "HeadUpSelectButton.h"
#include "Config/Config.h"
#include <Tools/Tools.h>

namespace HeadUpSelectButton {
    Class *get_class() {
        return FindClass("HeadUpSelectButton");
    }

    static std::unordered_map<Object *, long long> pendingActions; // Lưu trữ các hành động đang chờ xử lý

    void (*old_SetSprite)(Object *instance, String *spriteName);
    void SetSprite(Object *instance, String *spriteName) {
        old_SetSprite(instance, spriteName);
        std::string spriteNameStr = spriteName ? spriteName->to_string() : "";
        if (spriteNameStr.empty()) return;
        LOGI("SetSprite called with spriteName: %s", spriteNameStr.c_str());

        auto &insect = gPLConfig.insect;
        auto &collect = gPLConfig.collect;
        auto &farm = gPLConfig.farm;
        auto &ThapGa = gPLConfig.miniGame.ThapGa;

        if (!instance) return;

        if (spriteNameStr == "icon_trigger_harvest" && farm.isAutoClickCollect) {
            instance->invoke_method<void>("OnClick");
            instance->invoke_method<void>("OnDisable");
        }
        if (spriteNameStr == "icon_click" && ThapGa.isEnable) {
            pendingActions[instance] = Tools::getSystemMilliseconds();
        }

        if (spriteNameStr == "icon_trigger_pickup" &&
            ((insect.isAutoBatBo && insect.isNhatThe) ||
             (collect.isAutoDapDa || collect.isAutoNhatThe || collect.isAutoNguyenLieu))) {
            pendingActions[instance] = Tools::getSystemMilliseconds();
        }
    }

    void (*old_UpdatePosition)(Object *instance);
    void UpdatePosition(Object *instance) {
        old_UpdatePosition(instance);
        if (!instance) return;

        if (pendingActions.find(instance) == pendingActions.end()) {
            return; // Không có hành động nào đang chờ xử lý cho instance này
        }

        if (instance->get_field_value<bool>("isClosing")) {
            pendingActions.erase(instance); // Hủy bỏ hành động nếu nút đang đóng
            return;
        }

        if (Tools::getSystemMilliseconds() - pendingActions[instance] >= 1000) {
            instance->invoke_method<void>("OnClick");
            instance->invoke_method<void>("OnDisable");
            pendingActions.erase(instance);
        }
    }

}