#include "NetWebProtocol.h"
#include "CombineContent.h"

namespace NetWebProtocol {

void (*old_PID_SHOP_Buy)(Object *, Object *) = nullptr;
void (*old_PID_SHOP_BuyList)(Object *, Object *) = nullptr;

static void onShopAck(void (*oldFn)(Object *, Object *), Object *self, Object *reqAndRes) {
    bool ours = CombineContent::isIngredientBuyInFlight();
    if (oldFn) oldFn(self, reqAndRes);
    if (!reqAndRes || !ours) return;
    Object *res = reqAndRes->invoke_method<Object *>(OBF("get_Res"));
    CombineContent::onShopBuyAck(res ? res->invoke_method<int>(OBF("get_Result")) : -1);
}

bool RequestToShopBuyList(Object *productList) {
    Object *net = nullptr;
    IL2Cpp::Il2CppGetStaticFieldValue(OBF("NetworkSystem"), OBF("NetWeb"), &net);
    Class *cls = FindClass(OBF("PlayTogether.Network.Web.NetWebProtocol"));
    if (!net || !cls || !productList || !cls->find_method(OBF("RequestToShopBuyList"), 2)) return false;
    net->invoke_method<void>(OBF("RequestToShopBuyList"), productList, (Object *) nullptr);
    return true;
}

void onPID_SHOP_Buy(Object *self, Object *reqAndRes) { onShopAck(old_PID_SHOP_Buy, self, reqAndRes); }
void onPID_SHOP_BuyList(Object *self, Object *reqAndRes) { onShopAck(old_PID_SHOP_BuyList, self, reqAndRes); }

}
