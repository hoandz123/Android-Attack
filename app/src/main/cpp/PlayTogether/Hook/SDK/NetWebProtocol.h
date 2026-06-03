#ifndef SDK_NETWEBPROTOCOL_H
#define SDK_NETWEBPROTOCOL_H

#include <API/Il2CppApi.h>

namespace NetWebProtocol {

bool RequestToShopBuyList(Object *productList);

void onPID_SHOP_Buy(Object *self, Object *reqAndRes);
void onPID_SHOP_BuyList(Object *self, Object *reqAndRes);
extern void (*old_PID_SHOP_Buy)(Object *, Object *);
extern void (*old_PID_SHOP_BuyList)(Object *, Object *);

}

#endif
