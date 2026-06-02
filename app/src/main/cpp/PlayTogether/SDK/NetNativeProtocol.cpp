#include "PlayLog.h"
#include "NetNativeProtocol.h"
#include <Includes/Logger.h>
#include <Tools/Tools.h>
#include <API/Il2CppApi.h>
#include "enum/Item_Type.h"

namespace NetNativeProtocol {
    Class *get_class() {
        return FindClass("PlayTogether.Network.Native.NetNativeProtocol");
    }
    Object *GetNetNativeProtocol() {
        Object *NetNativeProtocol;
        IL2Cpp::Il2CppGetStaticFieldValue(OBF("NetworkSystem"), OBF("NetNative"), &NetNativeProtocol);
        if (!NetNativeProtocol) {
            LOGE("NetNativeProtocol not found");
        }
        return NetNativeProtocol;
    }
    void SendToItemRepair(long uid, Object *cb) {
        Object *instance = GetNetNativeProtocol();
        if (!instance) {
            LOGE("SendToItemRepair: NetNativeProtocol is null");
            return;
        }
        instance->invoke_method<void>("SendToItemRepair", uid, cb);
    }
    void SendToAchievementReward(int achievementID, int step) {
        Object *instance = GetNetNativeProtocol();
        if (!instance) {
            LOGE("SendToAchievementReward: NetNativeProtocol is null");
            return;
        }
        instance->invoke_method<void>("SendToAchievementReward", achievementID, step);
    }
    void SendToMissionReward(uint32_t missionID) {
        Object *instance = GetNetNativeProtocol();
        if (!instance) {
            LOGE("SendToMissionReward: NetNativeProtocol is null");
            return;
        }
        void *nullCb = nullptr;
        instance->invoke_method<void>("SendToMissionReward", missionID, nullCb);
    }
    void SendToStampReward() {
        Object *instance = GetNetNativeProtocol();
        if (!instance) {
            LOGE("SendToStampReward: NetNativeProtocol is null");
            return;
        }
        void *nullCb = nullptr;
        int stampTypeDaily = 1;
        bool isADReward = false;
        uint32_t eventID = 0;
        uint32_t stepRewardIndex = 0;
        uint32_t paidStampItemId = 0;
        instance->invoke_method<void>("SendToStampReward", nullCb, stampTypeDaily, isADReward, eventID, stepRewardIndex, paidStampItemId);
    }
    void SendToMailReceive(List<Object *> *mailChoiceList) {
        Object *instance = GetNetNativeProtocol();
        if (!instance) {
            LOGE("SendToMailReceive: NetNativeProtocol is null");
            return;
        }
        if (!mailChoiceList) {
            LOGE("SendToMailReceive: mailChoiceList is null");
            return;
        }
        void *nullCb = nullptr;
        instance->invoke_method<void>("SendToMailReceive", mailChoiceList, nullCb);
    }
    void SendToItemSell(void *sellItemList, Item_Type type, int targetNPC) {
        Object *instance = GetNetNativeProtocol();
        if (!instance) {
            LOGE("SendToAchievementReward: NetNativeProtocol is null");
            return;
        }
        void *nullVal = nullptr;
        instance->invoke_method<void>("SendToItemSell", sellItemList, type, targetNPC, nullVal);
    }
    void SendToTreasureDigging(int boxUid, int diggingCount) {
        Object *instance = GetNetNativeProtocol();
        if (!instance) {
            LOGE("SendToTreasureDigging: NetNativeProtocol is null");
            return;
        }
        instance->invoke_method<void>("SendToTreasureDigging", boxUid, diggingCount);
    }

    void SendToItemUse(long uid) {
        Object *instance = GetNetNativeProtocol();
        if (!instance) {
            LOGE("SendToItemUse: NetNativeProtocol is null");
            return;
        }
        instance->invoke_method<void>("SendToItemUse", uid);
    }
}

