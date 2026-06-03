#include "TableFishingBaitImpl.h"
#include "CacheUser.h"
#include "TableSystem.h"
#include "enum/eTableType.h"
#include <API/Il2CppApi.h>

namespace TableFishingBaitImpl {

unsigned int PickBestItemIdForAction(unsigned int actionId) {
    if (actionId == 0) return 0;
    Object *impl = TableSystem::GetTableUnit(eTableType::FishingBait);
    if (!impl) return 0;
    unsigned int bestId = 0;
    unsigned int bestOrder = 0;
    TableSystem::ForEachListRow(impl, 256, [&](Object *row) -> bool {
        Class *rowCls = row->get_class();
        if (!rowCls || !rowCls->find_method(OBF("get_EffectId"), 0) ||
            !rowCls->find_method(OBF("get_BaitItemId"), 0)) {
            return true;
        }
        unsigned int effect = row->invoke_method<unsigned int>(OBF("get_EffectId"));
        unsigned int group = row->invoke_method<unsigned int>(OBF("get_CheckActionGroup"));
        if (effect != actionId && group != actionId) return true;
        unsigned int baitItem = row->invoke_method<unsigned int>(OBF("get_BaitItemId"));
        if (baitItem == 0 || CacheUser::GetItemCount(baitItem, true) <= 0) return true;
        unsigned int order = row->invoke_method<unsigned int>(OBF("get_Order"));
        if (bestId == 0 || order >= bestOrder) {
            bestId = baitItem;
            bestOrder = order;
        }
        return true;
    });
    return bestId;
}

}
