#include "NetNativeProtocol.h"

namespace NetNativeProtocol {

Class *get_class() {
    return FindClass(OBF("PlayTogether.Network.Native.NetNativeProtocol"));
}

Object *get_Instance() {
    Object *net = nullptr;
    IL2Cpp::Il2CppGetStaticFieldValue(OBF("NetworkSystem"), OBF("NetNative"), &net);
    return net;
}

bool SendToCraftingCreate(int16_t slotId, unsigned int recipeId, Object *removeDict) {
    Object *self = get_Instance();
    Class *cls = get_class();
    if (!self || !cls || !removeDict || recipeId == 0 || slotId < 0) return false;
    if (!cls->find_method(OBF("SendToCraftingCreate"), 3)) return false;
    int16_t slotArg = slotId;
    self->invoke_method<void>(OBF("SendToCraftingCreate"), slotArg, recipeId, removeDict);
    return true;
}

bool SendToCraftingTimeDecrease(int16_t slotId, int decreaseType, long long adTransactionSeq) {
    Object *self = get_Instance();
    Class *cls = get_class();
    if (!self || !cls || slotId < 0) return false;
    if (!cls->find_method(OBF("SendToCraftingTimeDecrease"), 3)) return false;
    int16_t slotArg = slotId;
    int typeArg = decreaseType;
    self->invoke_method<void>(OBF("SendToCraftingTimeDecrease"), slotArg, typeArg, adTransactionSeq);
    return true;
}

bool SendToCraftingItemReward(int16_t slotId) {
    Object *self = get_Instance();
    Class *cls = get_class();
    if (!self || !cls || slotId < 0) return false;
    if (!cls->find_method(OBF("SendToCraftingItemReward"), 1)) return false;
    int16_t slotArg = slotId;
    self->invoke_method<void>(OBF("SendToCraftingItemReward"), slotArg);
    return true;
}

bool SendToItemCombine(unsigned int recipeId, Object *removeDict, int count) {
    Object *self = get_Instance();
    Class *cls = get_class();
    if (!self || !cls || !removeDict || !recipeId || count <= 0) return false;
    if (!cls->find_method(OBF("SendToItemCombine"), 4)) return false;
    self->invoke_method<void>(OBF("SendToItemCombine"), recipeId, removeDict, (Object *) nullptr, count);
    return true;
}

}
