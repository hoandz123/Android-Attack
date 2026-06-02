#include "PlayLog.h"
//
// Created by PC on 28/10/2025.
//

#include "NetWebProtocol.h"

namespace NetWebProtocol {
    Class* get_class() {
        return FindClass("PlayTogether.Network.Web.NetWebProtocol");
    }
    Object* GetNetWebProtocol() {
        Object *NetNativeProtocol;
        IL2Cpp::Il2CppGetStaticFieldValue(OBF("NetworkSystem"), OBF("NetWeb"), &NetNativeProtocol);
        if (!NetNativeProtocol) {
            LOGE("NetWebProtocol not found");
        }
        return NetNativeProtocol;
    }

    void RequestToShopBuy(int productID, int itemGroup, int subPriceItemID) {
        void (*_)(void *, int, int, void*, int) = (void (*)(void *, int, int, void*, int)) IL2Cpp::Il2CppGetMethodOffset("NetWebProtocol", "RequestToShopBuy", 4);
        if (_) {
            _(GetNetWebProtocol(), productID, itemGroup, nullptr, subPriceItemID);
        }
    }
    void RequestToShopBuyList(int productID, int itemCount) {
        void (*_)(void *, List<Object*>*, void*) = (void (*)(void *, List<Object*>*, void*)) IL2Cpp::Il2CppGetMethodOffset("NetWebProtocol", "RequestToShopBuyList", 2);
        if (_) {
            List<Object*>* productList = (List<Object*>*) FindClass("System.Collections.Generic.List<PlayTogetherGame.ShopBuyProduct>")->new_object();
            if (productList) {
                Object* newShopBuyProduct =  FindClass("PlayTogetherGame.ShopBuyProduct")->new_object();
                if (newShopBuyProduct) {
                    *(int *) ((uintptr_t) newShopBuyProduct + IL2Cpp::Il2CppGetFieldOffset("ShopBuyProduct", "<ProductId>k__BackingField")) = productID;
                    *(int *) ((uintptr_t) newShopBuyProduct + IL2Cpp::Il2CppGetFieldOffset("ShopBuyProduct", "<ProductCount>k__BackingField")) = itemCount;
                    *(int *) ((uintptr_t) newShopBuyProduct + IL2Cpp::Il2CppGetFieldOffset("ShopBuyProduct", "<PriceType>k__BackingField")) = 0;
                    *(int *) ((uintptr_t) newShopBuyProduct + IL2Cpp::Il2CppGetFieldOffset("ShopBuyProduct", "<PriceValue>k__BackingField")) = 0;
                    *(int *) ((uintptr_t) newShopBuyProduct + IL2Cpp::Il2CppGetFieldOffset("ShopBuyProduct", "<Enable>k__BackingField")) = 0;
                    productList->Add(newShopBuyProduct);
                    _(GetNetWebProtocol(), productList, nullptr);
                }
            }
        }
    }
};
