#ifndef SDK_NETWORKREWARDPACK_H
#define SDK_NETWORKREWARDPACK_H

#include <API/Il2CppApi.h>

namespace NetworkRewardPack {

Class *get_class();
Object *get_RewardList(Object *pack);
Object *get_ItemRewardList(Object *pack);
Object *get_LastCoinList(Object *pack);
Object *getFirstCoin(Object *pack);

}

#endif // SDK_NETWORKREWARDPACK_H
