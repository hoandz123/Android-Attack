#include "CombineContent.h"
#include "ContentSystem.h"
#include "CacheUser.h"
#include "NetNativeProtocol.h"
#include "NetWebProtocol.h"
#include "TableRecipeImpl.h"
#include "TableItemCraftingListImpl.h"
#include "TableIngredientGroupImpl.h"
#include "TableIngredientImpl.h"
#include "UserItemCraftingSlot.h"
#include "ProtocolA.h"
#include "NetworkRewardPack.h"
#include "ItemCraftingStartA.h"
#include "ItemCraftingRewardItemA.h"
#include "enum/Item_Type.h"
#include <map>
#include <API/Il2cpp_Struct.h>
#include <Tools/Tools.h>
#define LOGGER_TAG "ATTACK_CombineContent"
#include <Includes/Logger.h>

namespace CombineContent {

// Hook gốc — gọi trước handler mod để game cập nhật state/UI.
void (*old_PID_Crafting_Start)(Object *, Object *) = nullptr;
void (*old_PID_Crafting_TimeDecrease)(Object *, Object *) = nullptr;
void (*old_PID_Crafting_ItemReward)(Object *, Object *) = nullptr;
void (*old_PID_ITEM_Combine)(Object *, Object *) = nullptr;

namespace {

// --- Hằng số timing & mã FindIngredient (khớp CombineContent.IngredientFindResult) ---
struct CraftTiming {
    static constexpr long long timeoutMs = 30000;
    static constexpr long long rewardDelayMs = 500;
    static constexpr long long buyRetryDelayMs = 400;
};

struct IngredientFindResult {
    static constexpr int shortage = 0;
    static constexpr int ok = 4;
};

static const char *kGrp[] = {
        OBF("get_Ingredient1GroupId"), OBF("get_Ingredient2GroupId"), OBF("get_Ingredient3GroupId"),
        OBF("get_Ingredient4GroupId"), OBF("get_Ingredient5GroupId"),
};
static const char *kVal[] = {
        OBF("get_Ingredient1Value"), OBF("get_Ingredient2Value"), OBF("get_Ingredient3Value"),
        OBF("get_Ingredient4Value"), OBF("get_Ingredient5Value"),
};

// --- FSM chế mồi: phase + state craft đang chạy / mua NL đang chờ ---
enum class Phase : int { None, Create, Finish, WaitReward, Reward, Collect, CombineWait };

struct Pending {
    bool active = false;
    unsigned recipeId = 0, itemId = 0;
    long long sentMs = 0, waitUntilMs = 0;
    int16_t slotId = -1;
    Phase phase = Phase::None;
};

struct BuyPending {
    bool active = false;
    unsigned recipeId = 0, itemId = 0;
    int cookCount = 1;
    long long sentMs = 0, retryAfterMs = 0;
};

long long g_lastAckMs = 0;
Pending g_p;
BuyPending g_buy;

// --- Il2Cpp helpers: CombineContent instance, thời gian, reset pending ---
Class *cls() {
    static Class *c = FindClass(OBF("CombineContent"));
    return c;
}

Object *self() { return ContentSystem::get_Combine(); }

long long nowMs() { return Tools::getSystemMilliseconds(); }

bool pendingIs(Phase ph) { return g_p.active && g_p.phase == ph; }
void clearPending() { g_p = {}; }
void clearBuyPending() { g_buy = {}; }

const char *findResultName(int r) {
    static const char *n[] = {OBF("Shortage"), OBF("HasFavorite"), OBF("HasLock"), OBF("HasEquip"), OBF("OK")};
    return (r >= 0 && r < 5) ? n[r] : "?";
}

template<typename Fn>
void foreachIngredient(Object *recipe, Fn fn) {
    Class *rc = recipe ? recipe->get_class() : nullptr;
    if (!rc) return;
    for (int i = 0; i < 5; i++) {
        if (!rc->find_method(kGrp[i], 0) || !rc->find_method(kVal[i], 0)) continue;
        unsigned gid = recipe->invoke_method<unsigned int>(kGrp[i]);
        int per = recipe->invoke_method<int>(kVal[i]);
        if (gid && per > 0) fn(gid, per);
    }
}

// --- Slot workbench: tìm slot theo điều kiện (idle / ready), lấy UserItemCraftingSlot ---
template<typename Pred>
int16_t findSlot(Pred pred) {
    Object *s = self();
    Object *list = s ? s->get_field_object<Object *>(OBF("CraftingSlotList")) : nullptr;
    if (!list) return -1;
    auto *slots = (List<Object *> *) list;
    for (int i = 0, n = slots->get_Count(); i < n; i++) {
        Object *slot = slots->get_item(i);
        if (slot && pred(slot)) return UserItemCraftingSlot::get_SlotId(slot);
    }
    return -1;
}

Object *getSlot(int16_t slotId) {
    if (slotId < 0) return nullptr;
    Object *s = self();
    Object *list = s ? s->get_field_object<Object *>(OBF("CraftingSlotList")) : nullptr;
    if (list) {
        auto *slots = (List<Object *> *) list;
        for (int i = 0, n = slots->get_Count(); i < n; i++) {
            Object *slot = slots->get_item(i);
            if (slot && UserItemCraftingSlot::get_SlotId(slot) == slotId) return slot;
        }
    }
    if (!s || !cls() || !cls()->find_method(OBF("GetCraftingSlotInfo"), 1)) return nullptr;
    int slotArg = (int) slotId;
    Object *slot = s->invoke_method<Object *>(OBF("GetCraftingSlotInfo"), slotArg);
    return (slot && UserItemCraftingSlot::get_SlotId(slot) == slotId) ? slot : nullptr;
}

// --- Gom item túi làm ứng viên chọn NL (giống UI chế/nấu) ---
Object *candidateItems() {
    Object *cache = CacheUser::get_Instance(), *out = nullptr;
    Class *cc = CacheUser::get_class();
    static Class *listCls = FindClass(OBF("System.Collections.Generic.List<PlayTogether.UserItem>"));
    if (!cache || !cc || !listCls || !cc->find_method(OBF("GetItemList"), 1)) return nullptr;
    if (!(out = listCls->new_object())) return nullptr;
    static const Item_Type types[] = {
            Item_Type::Ingredient, Item_Type::Food, Item_Type::BaitItem, Item_Type::ItemGrowingMaterial,
            Item_Type::PlantsItem, Item_Type::OreItem, Item_Type::Junk, Item_Type::Fish,
    };
    auto *dst = (List<Object *> *) out;
    for (Item_Type t : types) {
        int typeArg = (int) t;
        Object *part = cache->invoke_method<Object *>(OBF("GetItemList"), typeArg);
        if (!part) continue;
        auto *src = (List<Object *> *) part;
        for (int i = 0, n = src->get_Count(); i < n; i++) dst->Add(src->get_item(i));
    }
    return out;
}

// --- Thiếu NL → tính product shop + số lượng, gửi PID_SHOP_BuyList ---
Object *buildShopBuyList(Object *recipe, int cookCount) {
    Object *s = self();
    Class *c = cls();
    if (!s || !recipe || cookCount <= 0 || !c || !c->find_method(OBF("GetIngredientSlotState"), 4)) return nullptr;
    static Class *prodCls = FindClass(OBF("PlayTogetherGame.ShopBuyProduct"));
    static Class *listCls = FindClass(OBF("System.Collections.Generic.List<PlayTogetherGame.ShopBuyProduct>"));
    if (!prodCls || !listCls) return nullptr;

    std::map<unsigned int, int> need;
    foreachIngredient(recipe, [&](unsigned gid, int per) {
        int needTotal = per * cookCount, have = 0, dummy = 0;
        s->invoke_method<int>(OBF("GetIngredientSlotState"), gid, needTotal, &dummy, &have);
        int buy = needTotal - have;
        if (buy <= 0) return;
        unsigned ing = TableIngredientGroupImpl::GetIngredientGroupFirstItemId(gid);
        unsigned pid = TableIngredientImpl::GetProductId(ing);
        if (!pid) { LOGD(OBF("Chế mồi NL không mua được: group=%u"), gid); return; }
        need[pid] += buy;
        LOGD(OBF("Chế mồi cần mua: group=%u product=%u buy=%d (need=%d have=%d)"), gid, pid, buy, needTotal, have);
    });
    if (need.empty()) return nullptr;

    Object *buyList = listCls->new_object();
    if (!buyList) return nullptr;
    for (const auto &kv : need) {
        Object *e = prodCls->new_object();
        if (!e) continue;
        e->invoke_method<void>(OBF("set_ProductId"), kv.first);
        e->invoke_method<void>(OBF("set_ProductCount"), kv.second);
        ((List<Object *> *) buyList)->Add(e);
    }
    return buyList;
}

// --- Combine tức thì (craftTime=0): FindIngredientItems × batch → RemoveItemInfo dict ---
Object *buildRemoveDict(Object *recipe, int cookCount, int *findFail) {
    if (findFail) *findFail = 0;
    Object *s = self(), *candidates = candidateItems();
    static Class *dictCls = FindClass(
            OBF("System.Collections.Generic.Dictionary<System.UInt32,System.Collections.Generic.List<PlayTogether.RemoveItemInfo>>"));
    Object *dict = dictCls ? dictCls->new_object() : nullptr;
    if (!s || !recipe || !dict || !candidates || !cls() || !cls()->find_method(OBF("FindIngredientItems"), 7)) return nullptr;
    if (((List<Object *> *) candidates)->get_Count() <= 0) return nullptr;
    int batch = cookCount > 0 ? cookCount : 1;

    bool ok = true;
    foreachIngredient(recipe, [&](unsigned gid, int per) {
        if (!ok) return;
        bool uI = false, uP = false, uG = false;
        Object *ref = dict;
        int needCount = per * batch;
        int r = s->invoke_method<int>(OBF("FindIngredientItems"), gid, needCount, candidates, &ref, &uI, &uP, &uG);
        dict = ref;
        if (r == IngredientFindResult::ok) return;
        ok = false;
        if (findFail) *findFail = r;
        LOGD(OBF("Chế mồi FindIngredientItems fail: group=%u result=%d (%s)"), gid, r, findResultName(r));
    });
    return ok ? dict : nullptr;
}

// --- Slot timed (craftTime>0): FindCraftingIngredient × 1 lần chế / slot ---
Object *buildSlotRemoveDict(Object *recipe, int *findFail) {
    if (findFail) *findFail = 0;
    Object *s = self(), *candidates = candidateItems();
    static Class *dictCls = FindClass(
            OBF("System.Collections.Generic.Dictionary<System.UInt32,System.Collections.Generic.List<PlayTogether.RemoveItemInfo>>"));
    Object *dict = dictCls ? dictCls->new_object() : nullptr;
    if (!s || !recipe || !dict || !candidates || !cls() || !cls()->find_method(OBF("FindCraftingIngredient"), 7))
        return nullptr;
    if (((List<Object *> *) candidates)->get_Count() <= 0) return nullptr;

    bool ok = true;
    foreachIngredient(recipe, [&](unsigned gid, int per) {
        if (!ok) return;
        bool uI = false, uP = false, uG = false;
        Object *ref = dict;
        if (s->invoke_method<bool>(OBF("FindCraftingIngredient"), gid, per, candidates, &ref, &uI, &uP, &uG)) {
            dict = ref;
            return;
        }
        ok = false;
        dict = ref;
        if (findFail) {
            int itemCount = 0, realCount = 0;
            *findFail = s->invoke_method<int>(OBF("GetIngredientSlotState"), gid, per, &itemCount, &realCount);
        }
        LOGD(OBF("Chế mồi FindCraftingIngredient fail: group=%u result=%d (%s)"), gid,
             findFail ? *findFail : -1, findResultName(findFail ? *findFail : -1));
    });
    return ok ? dict : nullptr;
}

// --- Trước slot create: sync theme event + CraftingSlotList nếu recipe thuộc theme ---
void prepSlotCraft(unsigned recipeId) {
    Object *s = self();
    Class *c = cls();
    if (!s || !c) return;
    Object *info = TableItemCraftingListImpl::GetCraftingInfo(recipeId);
    if (info && info->get_class() && info->get_class()->find_method(OBF("get_ThemeGroup"), 0)) {
        if (info->invoke_method<int16_t>(OBF("get_ThemeGroup")) != 0 && c->find_method(OBF("ForceThemeUpdate"), 0))
            s->invoke_method<void>(OBF("ForceThemeUpdate"));
    }
    if (c->find_method(OBF("CheckCraftingSlotInfo"), 0)) s->invoke_method<void>(OBF("CheckCraftingSlotInfo"));
}

// --- Chờ ack shop hoặc delay retry sau mua ---
bool buyInFlight() {
    long long t = nowMs();
    if (g_buy.retryAfterMs) return t < g_buy.retryAfterMs;
    if (!g_buy.active) return false;
    if (g_buy.sentMs && t - g_buy.sentMs > CraftTiming::timeoutMs) {
        LOGD(OBF("Chế mồi mua timeout recipe=%u"), g_buy.recipeId);
        clearBuyPending();
        return false;
    }
    return true;
}

// --- FSM helpers: đánh dấu pending, hẹn thu reward sau rewardDelayMs ---
void markPending(int16_t slotId, Phase phase, unsigned recipeId, unsigned itemId) {
    g_p = {true, recipeId, itemId, nowMs(), 0, slotId, phase};
}

void scheduleReward(int16_t slotId) {
    g_p.phase = Phase::WaitReward;
    g_p.slotId = slotId;
    g_p.waitUntilMs = nowMs() + CraftTiming::rewardDelayMs;
}

// --- Slot reward: cập nhật túi + slot local, bỏ dialog (hook ShowCraftingRewardDialog đã noop) ---
bool applySilentReward(Object *protocol) {
    Object *pack = ItemCraftingRewardItemA::get_Reward(protocol);
    if (!pack || !CacheUser::SetRewardPack(pack, false)) return false;
    Object *slot = ItemCraftingRewardItemA::get_Slot(protocol);
    if (!slot) return true;
    if (self() && cls() && cls()->find_method(OBF("UpdateCraftingSlot"), 2)) {
        self()->invoke_method<void>(OBF("UpdateCraftingSlot"), slot, NetworkRewardPack::getFirstCoin(pack));
        return true;
    }
    if (Object *local = getSlot(UserItemCraftingSlot::get_SlotId(slot)))
        UserItemCraftingSlot::copyFrom(local, slot);
    return true;
}

void logUserItems(Object *listObj) {
    if (!listObj) return;
    auto *list = (List<Object *> *) listObj;
    for (int i = 0, n = list->get_Count(); i < n; i++) {
        Object *it = list->get_item(i);
        if (!it || !it->get_class()) continue;
        LOGD(OBF("Chế mồi kết quả: itemId=%u x%d"),
             it->invoke_method<unsigned int>(OBF("get_ItemID")),
             it->invoke_method<int>(OBF("get_ItemCount")));
    }
}

void logReward(Object *protocol) {
    Object *pack = ItemCraftingRewardItemA::get_Reward(protocol);
    if (!pack) return;
    logUserItems(NetworkRewardPack::get_ItemRewardList(pack));
    logUserItems(NetworkRewardPack::get_RewardList(pack));
}

// --- Đồng bộ recipeId từ itemId mồi (cache UI có thể lệch) ---
unsigned resolveRecipe(unsigned recipeId, unsigned itemId) {
    if (!itemId) return recipeId;
    if (!TableRecipeImpl::GetRecipeForItem(itemId)) return 0;
    unsigned live = TableRecipeImpl::ResolveRecipeId(itemId);
    if (live && live != recipeId) LOGD(OBF("Chế mồi recipeId %u -> %u"), recipeId, live);
    return live;
}

// --- Gửi mua shop; sau ack (onShopBuyAck) retry craft sau buyRetryDelayMs ---
bool tryBuy(Object *recipe, unsigned recipeId, int cookCount, unsigned itemId) {
    if (g_buy.active || g_p.active || !recipe || !recipeId || cookCount <= 0) return false;
    Object *buyList = buildShopBuyList(recipe, cookCount);
    if (!buyList || !NetWebProtocol::RequestToShopBuyList(buyList)) {
        LOGD(OBF("Chế mồi mua fail: recipe=%u cook=%d"), recipeId, cookCount);
        return false;
    }
    g_buy = {true, recipeId, itemId, cookCount, nowMs(), 0};
    LOGD(OBF("Chế mồi mua shop: recipe=%u cook=%d"), recipeId, cookCount);
    return true;
}

// --- Luồng combine: buildRemoveDict → SendToItemCombine → chờ PID_ITEM_Combine ---
bool tryItemCombine(Object *recipe, unsigned recipeId, int cookCount, unsigned lookup) {
    int findFail = 0;
    Object *removeDict = buildRemoveDict(recipe, cookCount, &findFail);
    if (!removeDict) {
        if (findFail == IngredientFindResult::shortage && tryBuy(recipe, recipeId, cookCount, lookup)) return true;
        LOGD(OBF("Chế mồi combine fail: recipe=%u find=%d (%s)"), recipeId, findFail, findResultName(findFail));
        return false;
    }
    if (!NetNativeProtocol::SendToItemCombine(recipeId, removeDict, cookCount)) return false;
    markPending(-1, Phase::CombineWait, recipeId, lookup);
    LOGD(OBF("Chế mồi combine: recipe=%u x%d"), recipeId, cookCount);
    return true;
}

// --- Poll FSM: chờ mua NL / WaitReward→SendItemReward / timeout ---
bool inFlight() {
    if (buyInFlight()) return true;
    if (!g_p.active) return false;
    long long t = nowMs();
    if (g_p.phase == Phase::WaitReward) {
        if (t >= g_p.waitUntilMs) {
            g_p.phase = Phase::Reward;
            g_p.sentMs = t;
            NetNativeProtocol::SendToCraftingItemReward(g_p.slotId);
        }
        return true;
    }
    if (g_p.sentMs && t - g_p.sentMs > CraftTiming::timeoutMs) {
        if (g_p.phase == Phase::Finish && UserItemCraftingSlot::isReady(getSlot(g_p.slotId))) {
            scheduleReward(g_p.slotId);
            return true;
        }
        LOGD(OBF("Chế mồi timeout recipe=%u phase=%d"), g_p.recipeId, (int) g_p.phase);
        clearPending();
    }
    return g_p.active;
}

} // namespace

// --- Public: số lần chế tối đa game cho phép (NL + túi) ---
int CalculateMaxCookCount(unsigned int recipeId) {
    Object *s = self();
    return (s && cls() && cls()->find_method(OBF("CalculateMaxCookCount"), 1))
           ? s->invoke_method<int>(OBF("CalculateMaxCookCount"), recipeId) : 0;
}

bool isCraftInFlight() { return inFlight(); }
bool isIngredientBuyInFlight() { return buyInFlight(); }
long long lastCraftAckMs() { return g_lastAckMs; }

// --- Hook shop: mua NL xong → delay rồi TryInstantCombine retry ---
void onShopBuyAck(int result) {
    if (!g_buy.active) return;
    if (result != 0) {
        LOGD(OBF("Chế mồi mua fail recipe=%u %s"), g_buy.recipeId, ProtocolA::describe(result).c_str());
        clearBuyPending();
        clearPending();
        return;
    }
    g_buy.active = false;
    g_buy.retryAfterMs = nowMs() + CraftTiming::buyRetryDelayMs;
    g_lastAckMs = nowMs();
    clearPending();
    LOGD(OBF("Chế mồi mua xong recipe=%u"), g_buy.recipeId);
}

// --- Entry chế mồi (AutoFishing):
//     1) Thu slot ready nếu có
//     2) craftTime=0 → combine batch | craftTime>0 → slot create→finish→reward
//     3) Thiếu NL → tryBuy ---
bool TryInstantCombine(unsigned int recipeId, int cookCount, unsigned int itemId) {
    // Retry sau khi mua shop (g_buy.retryAfterMs)
    if (!g_buy.active && g_buy.retryAfterMs && nowMs() >= g_buy.retryAfterMs && g_buy.recipeId) {
        recipeId = g_buy.recipeId;
        itemId = g_buy.itemId;
        cookCount = g_buy.cookCount;
        clearBuyPending();
    }
    Object *s = self();
    if (!s || !recipeId) return false;

    // Ưu tiên thu món slot đã xong (kể cả craft tay / lần trước)
    if (!g_p.active) {
        int16_t ready = findSlot([](Object *slot) { return UserItemCraftingSlot::isReady(slot); });
        if (ready >= 0 && NetNativeProtocol::SendToCraftingItemReward(ready)) {
            markPending(ready, Phase::Collect, 0, 0);
            LOGD(OBF("Chế mồi thu slot=%d"), (int) ready);
            return true;
        }
    }

    recipeId = resolveRecipe(recipeId, itemId);
    if (!recipeId) return false;
    unsigned lookup = itemId ? itemId : TableRecipeImpl::FindItemIdByRecipeId(recipeId);
    Object *recipe = lookup ? TableRecipeImpl::GetRecipeForItem(lookup) : nullptr;
    if (!recipe) return false;

    int batch = cookCount > 0 ? cookCount : 1;
    unsigned craftTime = TableItemCraftingListImpl::GetCraftingTime(recipeId);
    // ItemCraftingList.CraftingTime==0: chế tức thì qua ItemCombine (1 gói × N)
    if (!TableItemCraftingListImpl::UsesSlotCraft(recipeId)) {
        LOGD(OBF("Chế mồi loại combine: recipe=%u (craftTime=%u)"), recipeId, craftTime);
        return tryItemCombine(recipe, recipeId, batch, lookup);
    }

    prepSlotCraft(recipeId);

    // Slot timed: 1 recipe / 1 slot / 1 lần create
    int16_t slotId = findSlot([](Object *slot) { return UserItemCraftingSlot::isIdle(slot); });
    if (slotId < 0) {
        LOGD(findSlot([](Object *slot) { return UserItemCraftingSlot::isReady(slot); }) >= 0
             ? OBF("Chế mồi: chờ thu slot") : OBF("Chế mồi: không có slot"));
        return false;
    }

    int findFail = 0;
    Object *removeDict = buildSlotRemoveDict(recipe, &findFail);
    if (!removeDict) {
        if (findFail == IngredientFindResult::shortage && tryBuy(recipe, recipeId, batch, lookup)) return true;
        LOGD(OBF("Chế mồi fail: recipe=%u find=%d (%s)"), recipeId, findFail, findResultName(findFail));
        return false;
    }
    if (!NetNativeProtocol::SendToCraftingCreate(slotId, recipeId, removeDict)) return false;
    markPending(slotId, Phase::Create, recipeId, lookup);
    LOGD(OBF("Chế mồi slot create: slot=%d recipe=%u time=%u"), (int) slotId, recipeId, craftTime);
    return true;
}

// --- Hook slot FSM: PID_Crafting_Start → TimeDecrease (FinishNow) hoặc thu ngay nếu ready ---
void onPID_Crafting_Start(Object *self, Object *protocol) {
    old_PID_Crafting_Start(self, protocol);
    if (!protocol || !pendingIs(Phase::Create)) return;
    int16_t slotId = g_p.slotId;
    if (Object *slot = ItemCraftingStartA::get_UserItemCraftingSlot(protocol)) {
        int16_t id = UserItemCraftingSlot::get_SlotId(slot);
        if (id >= 0) slotId = id;
    }
    int r = ProtocolA::get_Result(protocol);
    if (r != 0) {
        LOGD(OBF("Chế mồi start fail slot=%d %s"), (int) slotId, ProtocolA::describe(r).c_str());
        g_lastAckMs = nowMs();
        clearPending();
        return;
    }
    g_p.slotId = slotId;
    LOGD(OBF("Chế mồi start OK slot=%d"), (int) slotId);
    if (UserItemCraftingSlot::isReady(getSlot(slotId))) {
        scheduleReward(slotId);
        return;
    }
    g_p.phase = Phase::Finish;
    g_p.sentMs = nowMs();
    if (!NetNativeProtocol::SendToCraftingTimeDecrease(slotId, 1, (long long) 0)) clearPending();
}

// --- Hook slot: PID_Crafting_TimeDecrease OK → hẹn thu reward ---
void onPID_Crafting_TimeDecrease(Object *self, Object *protocol) {
    old_PID_Crafting_TimeDecrease(self, protocol);
    if (!protocol || !pendingIs(Phase::Finish)) return;
    int r = ProtocolA::get_Result(protocol);
    if (r != 0) {
        LOGD(OBF("Chế mồi finish fail %s"), ProtocolA::describe(r).c_str());
        clearPending();
        return;
    }
    LOGD(OBF("Chế mồi finish OK slot=%d"), (int) g_p.slotId);
    scheduleReward(g_p.slotId);
}

void onShowCraftingRewardDialog(Object * /*self*/, Object * /*rewardList*/) {}

// --- Hook slot: PID_Crafting_ItemReward → applySilentReward, bỏ popup ---
void onPID_Crafting_ItemReward(Object *self, Object *protocol) {
    old_PID_Crafting_ItemReward(self, protocol);
    bool ours = g_p.active && (g_p.phase == Phase::Reward || g_p.phase == Phase::Collect);
    int16_t slotId = g_p.slotId;
    unsigned itemId = g_p.itemId;
    int r = ProtocolA::get_Result(protocol);
    clearPending();
    if (!ours) { if (r == 0) logReward(protocol); return; }
    if (r != 0) {
        LOGD(OBF("Chế mồi reward fail slot=%d %s"), (int) slotId, ProtocolA::describe(r).c_str());
        return;
    }
    if (!applySilentReward(protocol)) return;
    g_lastAckMs = nowMs();
    LOGD(OBF("Chế mồi reward OK slot=%d item=%u"), (int) slotId, itemId);
    logReward(protocol);
}

// --- Hook combine: PID_ITEM_Combine → ResCombineItem (game gốc) + log kết quả ---
void onPID_ITEM_Combine(Object *self, Object *protocol) {
    old_PID_ITEM_Combine(self, protocol);
    if (!protocol || !pendingIs(Phase::CombineWait)) return;
    unsigned recipeId = g_p.recipeId;
    unsigned itemId = g_p.itemId;
    int r = ProtocolA::get_Result(protocol);
    clearPending();
    if (r != 0) {
        LOGD(OBF("Chế mồi combine fail recipe=%u %s"), recipeId, ProtocolA::describe(r).c_str());
        g_lastAckMs = nowMs();
        return;
    }
    g_lastAckMs = nowMs();
    if (protocol->get_class() && protocol->get_class()->find_method(OBF("get_SuccessCount"), 0)) {
        LOGD(OBF("Chế mồi combine OK recipe=%u success=%u item=%u"),
             recipeId, protocol->invoke_method<unsigned int>(OBF("get_SuccessCount")), itemId);
    } else {
        LOGD(OBF("Chế mồi combine OK recipe=%u item=%u"), recipeId, itemId);
    }
    if (protocol->get_class() && protocol->get_class()->find_method(OBF("get_ResultItem"), 0))
        logUserItems(protocol->invoke_method<Object *>(OBF("get_ResultItem")));
}

}
