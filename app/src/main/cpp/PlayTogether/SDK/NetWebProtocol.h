//
// Created by PC on 28/10/2025.
//

#pragma once
#include <API/Il2CppApi.h>

namespace NetWebProtocol {
    Class* get_class();
    Object* GetNetWebProtocol();
    void RequestToShopBuy(int productID, int itemGroup = 0, int subPriceItemID = 0);
    void RequestToShopBuyList(int productID, int itemCount = 1);
};

