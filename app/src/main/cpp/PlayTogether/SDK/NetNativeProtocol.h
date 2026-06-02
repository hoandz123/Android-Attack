#ifndef SDK_NETNATIVEPROTOCOL_H
#define SDK_NETNATIVEPROTOCOL_H

#include <cstdint>
#include <API/Il2CppApi.h>
#include "enum/Item_Type.h"

namespace NetNativeProtocol {
    Class *get_class();
    Object *GetNetNativeProtocol();
    void SendToItemUse(long uid);
    void SendToItemRepair(long uid, Object *cb);
    void SendToAchievementReward(int achievementID, int step);
    void SendToItemSell(void *sellItemList, Item_Type type, int targetNPC);
    void SendToTreasureDigging(int boxUid, int diggingCount);
}

#endif // SDK_NETNATIVEPROTOCOL_H

