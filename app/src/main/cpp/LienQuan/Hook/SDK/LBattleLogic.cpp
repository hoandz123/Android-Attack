#include "LBattleLogic.h"
#include "LActorRoot.h"
#include "LFramework.h"
#include "LLogicCore.h"
#include <Includes/obfuscate.h>
#include <vector>

namespace lienquan {
namespace LBattleLogic {

namespace {

struct ResolveResult {
    Object *battle = nullptr;
    Object *desk = nullptr;
    bool fromMain = false;
};

struct Candidate {
    Object *battle = nullptr;
    Object *desk = nullptr;
    bool fromMain = false;
    bool hasMgr = false;
    int tier = 0;
};

Object *BattleFromDesk(Object *desk, bool &fromMain) {
    fromMain = false;
    if (!desk) return nullptr;
    Object *battle = LFramework::GetDeskBattleLogic(desk);
    if (battle) return battle;
    if (LFramework::IsFramework(desk)) {
        fromMain = true;
        return LFramework::GetMainBattleLogic(desk);
    }
    if (LFramework::IsStateSyncDesk(desk)) return LFramework::GetRealBattleLogic(desk);
    return nullptr;
}

bool BattleHasActorMgr(Object *battle) {
    if (!battle) return false;
    Object *mgr = battle->invoke_method<Object *>(OBF("get_gameActorMgr"));
    return mgr != nullptr;
}

int CandidateTier(bool fromInst, bool fromMain) {
    if (fromInst && fromMain) return 3;
    if (fromInst) return 2;
    return 1;
}

void AddCandidateFromDesk(Object *desk, bool fromInst, std::vector<Candidate> &cands) {
    if (!desk) return;
    bool fromMain = false;
    Object *battle = BattleFromDesk(desk, fromMain);
    if (!battle) return;
    Candidate c{};
    c.battle = battle;
    c.desk = desk;
    c.fromMain = fromMain;
    c.hasMgr = BattleHasActorMgr(battle);
    c.tier = CandidateTier(fromInst, fromMain);
    cands.push_back(c);
}

ResolveResult PickBestCandidate(const std::vector<Candidate> &cands) {
    ResolveResult r{};
    if (cands.empty()) return r;
    bool preferMgr = false;
    for (const Candidate &c : cands) {
        if (c.hasMgr) {
            preferMgr = true;
            break;
        }
    }
    auto try_pick = [&](bool requireMgr) -> bool {
        int bestTier = -1;
        size_t bestIdx = 0;
        for (size_t i = 0; i < cands.size(); ++i) {
            const Candidate &c = cands[i];
            if (requireMgr && !c.hasMgr) continue;
            if (c.tier > bestTier) {
                bestTier = c.tier;
                bestIdx = i;
            }
        }
        if (bestTier < 0) return false;
        const Candidate &w = cands[bestIdx];
        r.battle = w.battle;
        r.desk = w.desk;
        r.fromMain = w.fromMain;
        return true;
    };
    if (!try_pick(preferMgr) && preferMgr) try_pick(false);
    return r;
}

ResolveResult ResolveBattle() {
    ResolveResult r{};
    std::vector<Candidate> cands;
    if (!LLogicCore::GetInstance()) return r;
    const int n = LLogicCore::DeskCount();
    for (int i = 0; i < n; ++i) {
        Object *desk = LLogicCore::DeskAt(i);
        AddCandidateFromDesk(desk, true, cands);
    }
    // curUpdatingDesk racy/thường null trên release: chỉ là ứng viên last-resort tier thấp nhất.
    Object *curDesk = LLogicCore::GetCurUpdatingDesk();
    AddCandidateFromDesk(curDesk, false, cands);
    return PickBestCandidate(cands);
}

} // namespace

Object *Get() { return ResolveBattle().battle; }

Object *GetGameActorMgr() {
    Object *battle = Get();
    if (!battle) return nullptr;
    return battle->invoke_method<Object *>(OBF("get_gameActorMgr"));
}

bool HasActorMgr() { return GetGameActorMgr() != nullptr; }

unsigned int GetHostPlayerId() {
    Object *battle = Get();
    if (!battle) return 0;
    return battle->invoke_method<unsigned int>(OBF("get_HostPlayerId"));
}

Object *GetHostActorRoot() {
    Object *battle = Get();
    if (!battle) return nullptr;
    Object *hostMod = battle->invoke_method<Object *>(OBF("get_HostPlayer"));
    if (!hostMod) return nullptr;
    Object *handle = hostMod->invoke_method<Object *>(OBF("GetHostPlayerActorRoot"));
    return LActorRoot::FromPoolHandle(handle);
}

} // namespace LBattleLogic
} // namespace lienquan
