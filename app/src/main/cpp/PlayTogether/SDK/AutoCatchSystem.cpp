#include "PlayLog.h"
#include "AutoCatchSystem.h"
#include "Config/Config.h"
#include "System/DateTime.h"
#include <Tools/Tools.h>

namespace AutoCatchSystem {
    Class *get_content_class() {
        return FindClass("ContentSystem");
    }

    Object *get_Instance() {
        Class *cls = get_content_class();
        if (!cls) return nullptr;
        return cls->get_static_field_value<Object *>("AutoCatch");
    }

    static uint32_t find_zone_id(Object *content, int catchType) {
        List<Object *> *zoneInfos = content->get_field_object<List<Object *> *>("_autoCatchZoneInfos");
        if (!zoneInfos) return 0;
        int count = zoneInfos->get_Count();
        for (int i = 0; i < count; i++) {
            Object *zone = zoneInfos->get_item(i);
            if (!zone) continue;
            int zoneType = zone->get_field_value<int>("AutoCatchItemType");
            if (zoneType == catchType) {
                return zone->get_field_value<uint32_t>("ZoneID");
            }
        }
        return 0;
    }

    void Update() {
        auto &cfg = gPLConfig.autoCatch;
        if (!cfg.isAuto) return;

        Object *content = get_Instance();
        if (!content) return;

        RATE_LIMIT(2000);
        System::DateTime now = System::DateTime::Now();
        int catchType = cfg.catchType;

        if (cfg.isRetrieve || cfg.isCheck) {
            List<Object *> *itemList = content->invoke_method<List<Object *> *>("get_AutoCatchItemList");
            if (itemList) {
                int count = itemList->get_Count();
                for (int i = 0; i < count; i++) {
                    Object *item = itemList->get_item(i);
                    if (!item) continue;
                    System::DateTime endTime = item->invoke_method<System::DateTime>("get_EndTime");
                    if (endTime > now) continue;
                    uint32_t zoneId = item->invoke_method<uint32_t>("get_ZoneId");
                    int16_t slot = item->invoke_method<int16_t>("get_Slot");
                    bool isAvailable = item->invoke_method<bool>("get_IsAvailable");
                    void *nullCb = nullptr;
                    if (cfg.isRetrieve && isAvailable) {
                        LOGI("AutoCatchItemRetrieve zone=%u slot=%d", zoneId, (int) slot);
                        content->invoke_method<void>("AutoCatchItemRetrieve", zoneId, slot, catchType, nullCb);
                        return;
                    }
                    if (cfg.isCheck && !isAvailable) {
                        LOGI("AutoCatchItemCheck zone=%u slot=%d", zoneId, (int) slot);
                        content->invoke_method<void>("AutoCatchItemCheck", zoneId, slot, nullCb);
                        return;
                    }
                }
            }
        }

        if (!cfg.isInstall || cfg.mainItemId < 1) return;
        if (!content->invoke_method<bool>("IsPossibleInstallAutoCatchItem", catchType)) return;

        List<Object *> *activeList = content->invoke_method<List<Object *> *>("GetAutoCatchItemListByType", catchType);
        if (activeList && activeList->get_Count() > 0) return;

        uint32_t zoneId = find_zone_id(content, catchType);
        if (zoneId < 1) return;

        int16_t slotNum = 0;
        LOGI("AutoCatchItemInstall zone=%u main=%u sub=%u", zoneId, cfg.mainItemId, cfg.subItemId);
        content->invoke_method<void>("AutoCatchItemInstall", zoneId, cfg.mainItemId, cfg.subItemId, slotNum, catchType);
    }
}
