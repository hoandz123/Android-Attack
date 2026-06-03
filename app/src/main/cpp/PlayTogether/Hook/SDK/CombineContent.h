#pragma once

#include <API/Il2CppApi.h>
#include <cstdint>

namespace CombineContent {

int CalculateMaxCookCount(unsigned int recipeId);
bool TryInstantCombine(unsigned int recipeId, int cookCount, unsigned int itemId = 0);
bool isCraftInFlight();
bool isIngredientBuyInFlight();
long long lastCraftAckMs();
void onShopBuyAck(int result);

void onPID_Crafting_Start(Object *self, Object *protocol);
void onPID_Crafting_TimeDecrease(Object *self, Object *protocol);
void onPID_Crafting_ItemReward(Object *self, Object *protocol);
void onPID_ITEM_Combine(Object *self, Object *protocol);
void onShowCraftingRewardDialog(Object *self, Object *rewardList);
extern void (*old_PID_Crafting_Start)(Object *, Object *);
extern void (*old_PID_Crafting_TimeDecrease)(Object *, Object *);
extern void (*old_PID_Crafting_ItemReward)(Object *, Object *);
extern void (*old_PID_ITEM_Combine)(Object *, Object *);

}
