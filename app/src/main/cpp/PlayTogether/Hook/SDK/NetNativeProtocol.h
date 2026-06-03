#ifndef SDK_NETNATIVEPROTOCOL_H
#define SDK_NETNATIVEPROTOCOL_H

#include <API/Il2CppApi.h>
#include <cstdint>

namespace NetNativeProtocol {

Class *get_class();
Object *get_Instance();
bool SendToCraftingCreate(int16_t slotId, unsigned int recipeId, Object *removeDict);
bool SendToCraftingTimeDecrease(int16_t slotId, int decreaseType, long long adTransactionSeq);
bool SendToCraftingItemReward(int16_t slotId);

}

#endif // SDK_NETNATIVEPROTOCOL_H
