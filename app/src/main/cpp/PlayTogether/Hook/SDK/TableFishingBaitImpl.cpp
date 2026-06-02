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
    Class *implCls = impl->get_class();
    if (!implCls || !implCls->find_method(OBF("get_List"), 0)) return 0;
    Object *list = impl->invoke_method<Object *>(OBF("get_List"));
    if (!list) return 0;
    Class *listCls = list->get_class();
    if (!listCls || !listCls->find_method(OBF("get_Count"), 0) || !listCls->find_method(OBF("get_Item"), 1)) return 0;
    int count = list->invoke_method<int>(OBF("get_Count"));
    unsigned int bestId = 0;
    unsigned int bestOrder = 0;
    for (int i = 0; i < count; i++) {
        Object *row = list->invoke_method<Object *>(OBF("get_Item"), i);
        if (!row) continue;
        Class *rowCls = row->get_class();
        if (!rowCls || !rowCls->find_method(OBF("get_EffectId"), 0) || !rowCls->find_method(OBF("get_BaitItemId"), 0)) continue;
        unsigned int effect = row->invoke_method<unsigned int>(OBF("get_EffectId"));
        unsigned int group = row->invoke_method<unsigned int>(OBF("get_CheckActionGroup"));
        if (effect != actionId && group != actionId) continue;
        unsigned int baitItem = row->invoke_method<unsigned int>(OBF("get_BaitItemId"));
        if (baitItem == 0 || CacheUser::GetItemCount(baitItem, true) <= 0) continue;
        unsigned int order = row->invoke_method<unsigned int>(OBF("get_Order"));
        if (bestId == 0 || order >= bestOrder) {
            bestId = baitItem;
            bestOrder = order;
        }
    }
    return bestId;
}

}
