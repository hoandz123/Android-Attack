#include "EspRuntime.h"
#include "../Config/Config.h"
#include "../Data/HeroIcon/HeroIcon.h"
#include "SDK/ActorManager.h"
#include "SDK/CBattleSystem.h"
#include "SDK/LActorRoot.h"
#include "SDK/LGameActorMgr.h"
#include "SDK/LBattleLogic.h"
#include "SDK/MiniMapSysUT.h"
#include "SDK/MinimapSys.h"
#include <API/Il2CppApi.h>
#include <API/game/UnityEngine/Camera.h>
#include <GameUI/EspGUI.h>
#include <GameUI/GameViewport.h>
#include <Includes/obfuscate.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstring>
#include <mutex>
#include <string>
#include <unordered_map>

#include <stb_image.h>

namespace lienquan {
namespace EspRuntime {
namespace {

struct Frame {
    bool hasMyWorld = false;
    int targetCount = 0;
    Vector3 myWorld{};
    Vector3 targetWorld[kMaxTargets]{};
    unsigned int targetObjId[kMaxTargets]{};
    LActorRoot::ActorInfo targetInfo[kMaxTargets]{};
    bool targetInfoValid[kMaxTargets]{};
    LActorRoot::SkillCooldownInfo targetCd[kMaxTargets]{};
    bool targetCdValid[kMaxTargets]{};
};

struct IconCache {
    bool ready = false;
    int width = 0;
    int height = 0;
    uint32_t version = 0;
    std::vector<uint8_t> rgba;
};

Snapshot gBuffers[2]{};
std::atomic<int> gActive{0};
std::mutex gSnapMutex;
std::mutex gIconMutex;
std::unordered_map<std::string, IconCache> gIcons;
EspGUI::PosSmoother gSmoother;
int64_t gLastMs = 0;

int64_t NowMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

float SampleDeltaTime() {
    const int64_t now = NowMs();
    float dt = gLastMs > 0 ? static_cast<float>(now - gLastMs) / 1000.f : 1.f / 60.f;
    gLastMs = now;
    if (dt <= 0.f) dt = 1.f / 60.f;
    if (dt > 0.1f) dt = 0.1f;
    return dt;
}

void CopyText(char *dst, size_t n, const char *src) {
    if (!dst || n == 0 || !src) return;
    const size_t len = std::min(n - 1, std::strlen(src));
    std::memcpy(dst, src, len);
    dst[len] = '\0';
}

bool HasObjId(unsigned int id, const unsigned int *ids, int n) {
    for (int i = 0; i < n; ++i) if (ids[i] == id) return true;
    return false;
}

bool LinkerOk(Object *o) {
    if (!o) return false;
    const auto u = reinterpret_cast<uintptr_t>(o);
    if ((u & 3) != 0 || u <= 0x10000) return false;
    try {
        return o->get_class() != nullptr;
    } catch (...) {
        return false;
    }
}

unsigned int LinkerObjId(Object *linker) {
    if (!LinkerOk(linker)) return 0;
    if (!linker->get_class()->find_method(OBF("get_ObjID"), 0)) return 0;
    try {
        return linker->invoke_method<unsigned int>(OBF("get_ObjID"));
    } catch (...) {
        return 0;
    }
}

const HeroIcon::Entry *LookupEmbeddedIcon(const std::string &displayName) {
    return HeroIcon::FindByNameLoose(displayName.c_str());
}

bool DecodeEmbeddedIcon(const HeroIcon::Entry &entry, std::vector<uint8_t> &rgba, int &width, int &height) {
    rgba.clear();
    width = height = 0;
    if (!entry.iconBytes || entry.iconSize == 0) return false;
    int w = 0, h = 0, ch = 0;
    unsigned char *px = stbi_load_from_memory(entry.iconBytes, static_cast<int>(entry.iconSize), &w, &h, &ch, 4);
    if (!px || w <= 0 || h <= 0) return false;
    rgba.assign(px, px + static_cast<size_t>(w) * h * 4);
    stbi_image_free(px);
    width = w;
    height = h;
    return true;
}

void EnsureIconPixels(const char *fieldKey, const HeroIcon::Entry &entry) {
    if (!fieldKey || !fieldKey[0]) return;
    {
        std::lock_guard<std::mutex> lock(gIconMutex);
        auto it = gIcons.find(fieldKey);
        if (it != gIcons.end() && it->second.ready) return;
    }

    std::vector<uint8_t> rgba;
    int w = 0, h = 0;
    if (!DecodeEmbeddedIcon(entry, rgba, w, h)) return;

    std::lock_guard<std::mutex> lock(gIconMutex);
    IconCache &cache = gIcons[fieldKey];
    cache.ready = true;
    cache.width = w;
    cache.height = h;
    cache.rgba.swap(rgba);
    ++cache.version;
}

void FormatInfoLabel(const LActorRoot::ActorInfo &info, const char *iconKey, bool showDistance, bool showHpPercent, const Vector3 *myWorld, const Vector3 *targetWorld, char *dst, size_t n) {
    if (!dst || n == 0) return;
    const char *name = "?";
    if (iconKey && iconKey[0]) {
        const HeroIcon::Entry *entry = HeroIcon::FindByFieldName(iconKey);
        if (entry && entry->displayName && entry->displayName[0]) name = entry->displayName;
    }
    char extra[56]{};
    int extraLen = 0;
    if (info.hasSoulLevel) extraLen += snprintf(extra + extraLen, sizeof(extra) - (size_t) extraLen, extraLen ? " Lv%d" : "Lv%d", info.soulLevel);
    if (info.hasHp && info.maxHp > 0) {
        const int pct = info.hp * 100 / info.maxHp;
        if (showHpPercent) extraLen += snprintf(extra + extraLen, sizeof(extra) - (size_t) extraLen, extraLen ? " %d%%" : "%d%%", pct);
        else extraLen += snprintf(extra + extraLen, sizeof(extra) - (size_t) extraLen, extraLen ? " %d/%d" : "%d/%d", info.hp, info.maxHp);
    } else if (info.hasHp) {
        extraLen += snprintf(extra + extraLen, sizeof(extra) - (size_t) extraLen, extraLen ? " %d/%d" : "%d/%d", info.hp, info.maxHp);
    }
    if (showDistance && myWorld && targetWorld) {
        const float dist = Vector3::Distance(*myWorld, *targetWorld);
        extraLen += snprintf(extra + extraLen, sizeof(extra) - (size_t) extraLen, " %.0fm", dist);
    }
    if (info.hasDeadState && info.isDead) extraLen += snprintf(extra + extraLen, sizeof(extra) - (size_t) extraLen, " [Chết]");
    snprintf(dst, n, "%s%s", name, extra);
}

int ClampDisplaySec(int cdSec) {
    if (cdSec <= 0) return 0;
    return cdSec > 999 ? 999 : cdSec;
}

void FillInfoMeta(InfoItem &item, const LActorRoot::ActorInfo &info) {
    item.hasHp = info.hasHp;
    item.hp = info.hp;
    item.maxHp = info.maxHp;
    item.isDead = info.hasDeadState && info.isDead;
    item.lowHp = false;
    if (item.hasHp && item.maxHp > 0 && !item.isDead) item.lowHp = item.hp * 100 / item.maxHp < 25;
}

void FillInfoCooldowns(InfoItem &item, const LActorRoot::SkillCooldownInfo &cd) {
    item.hasCooldowns = false;
    if (!gLQConfig.esp.showCooldowns || !cd.hasData) return;
    int validN = 0;
    for (int i = 0; i < LActorRoot::kSkillCooldownSlots; ++i) {
        item.cooldownSlots[i] = cd.slots[i];
        if (item.cooldownSlots[i].valid) {
            item.cooldownSlots[i].cdSec = ClampDisplaySec(item.cooldownSlots[i].cdSec);
            validN++;
        }
    }
    item.hasCooldowns = validN > 0;
}

bool NeedsInfoItem() {
    return gLQConfig.esp.showInfo || gLQConfig.esp.showHpBar || gLQConfig.esp.showCooldowns;
}

bool NeedsWorldOverlay() {
    return gLQConfig.esp.enabled || NeedsInfoItem() || gLQConfig.esp.offscreenArrow;
}

void SetInfoItem(Snapshot &snap, int idx, float x, float y, const LActorRoot::ActorInfo &info, const char *iconKey, bool showDistance, bool showHpPercent, const Vector3 *myWorld, const Vector3 *targetWorld, const LActorRoot::SkillCooldownInfo *cd, bool showText) {
    if (idx < 0 || idx >= kMaxTargets) return;
    InfoItem &item = snap.infoItems[idx];
    item.valid = true;
    item.x = x;
    item.y = y;
    FillInfoMeta(item, info);
    if (cd) FillInfoCooldowns(item, *cd);
    if (showText) FormatInfoLabel(info, iconKey, showDistance, showHpPercent, myWorld, targetWorld, item.text, sizeof(item.text));
    else item.text[0] = '\0';
}

void CollectHero(Frame &f, float dt, Object *root, unsigned int hostPlayerId, int hostEnemyCamp, unsigned int hostObjId, bool myFromHost, unsigned int *seen, int &seenN) {
    if (!root) return;
    LActorRoot::ActorInfo info{};
    const bool hasInfo = LActorRoot::ReadActorInfo(root, info);
    const unsigned int objId = LActorRoot::GetObjID(root);
    if (objId && HasObjId(objId, seen, seenN)) return;
    if (myFromHost && hostObjId != 0 && objId == hostObjId) {
        if (objId && seenN < kMaxTargets) seen[seenN++] = objId;
        return;
    }
    const Vector3 world = LActorRoot::GetWorldPosition(root);
    const Vector3 pos = gSmoother.get(gSmoother.keyFor(objId), world, dt);
    const unsigned int playerId = LActorRoot::GetActorPlayerId(root);
    if (hostPlayerId != 0 && playerId == hostPlayerId) {
        if (!f.hasMyWorld) {
            f.myWorld = pos;
            f.hasMyWorld = true;
        }
        if (objId && seenN < kMaxTargets) seen[seenN++] = objId;
        return;
    }
    const int enemyCamp = LActorRoot::GetEnemyCamp(root);
    const bool teamKnown = hostEnemyCamp >= 0 && enemyCamp >= 0;
    const bool sameTeam = teamKnown && enemyCamp == hostEnemyCamp;
    if (sameTeam) return;
    if (hasInfo) {
        const bool dead = info.hasDeadState && info.isDead;
        const bool hpZero = info.hasHp && info.hp <= 0;
        if (dead || hpZero) {
            if (objId && seenN < kMaxTargets) seen[seenN++] = objId;
            return;
        }
    }
    if (f.targetCount >= kMaxTargets) return;
    const int idx = f.targetCount;
    f.targetObjId[idx] = objId;
    f.targetWorld[idx] = pos;
    if (hasInfo) {
        f.targetInfo[idx] = info;
        f.targetInfoValid[idx] = true;
    }
    if (gLQConfig.esp.showCooldowns) {
        LActorRoot::SkillCooldownInfo cd{};
        if (LActorRoot::ReadSkillCooldowns(root, cd)) {
            f.targetCd[idx] = cd;
            f.targetCdValid[idx] = true;
        }
    }
    f.targetCount++;
    if (objId && seenN < kMaxTargets) seen[seenN++] = objId;
}

bool BuildFrame(Frame &f, float dt, Object *gameActorMgr) {
    f = {};
    gSmoother.tick(dt);
    const unsigned int hostPlayerId = LBattleLogic::GetHostPlayerId();
    const int heroN = gameActorMgr ? LGameActorMgr::HeroCount(gameActorMgr) : 0;
    if (!gameActorMgr || heroN <= 0) return false;

    unsigned int hostObjId = 0;
    int hostEnemyCamp = -1;
    bool myFromHost = false;
    Object *hostRoot = LBattleLogic::GetHostActorRoot();
    if (hostRoot) {
        hostObjId = LActorRoot::GetObjID(hostRoot);
        f.myWorld = gSmoother.get(gSmoother.keyFor(hostObjId), LActorRoot::GetWorldPosition(hostRoot), dt);
        f.hasMyWorld = true;
        hostEnemyCamp = LActorRoot::GetEnemyCamp(hostRoot);
        myFromHost = true;
    }

    unsigned int seen[kMaxTargets]{};
    int seenN = 0;
    for (int i = 0; i < heroN; ++i) {
        CollectHero(f, dt, LActorRoot::HeroAt(i, gameActorMgr), hostPlayerId, hostEnemyCamp, hostObjId, myFromHost, seen, seenN);
    }
    return f.hasMyWorld || f.targetCount > 0;
}

bool InRect(float x, float y, float sw, float sh, float margin) {
    return x >= -margin && y >= -margin && x <= sw + margin && y <= sh + margin;
}

bool TargetOnScreen(const Vector3 &world, float sw, float sh, float margin) {
    const Vector3 sp = UnityEngine::Camera::StaticWorldToScreenPoint(world);
    if (sp.z <= 0.1f) return false;
    return InRect(sp.x, sh - sp.y, sw, sh, margin);
}

bool TryOffscreenArrow(const Vector3 &target, float sw, float sh, float margin, float &outX, float &outY, float &outAngle) {
    const Vector3 sp = UnityEngine::Camera::StaticWorldToScreenPoint(target);
    if (sp.z <= 0.1f) return false;
    const float sx = sp.x;
    const float sy = sh - sp.y;
    if (InRect(sx, sy, sw, sh, margin)) return false;
    const float cx = sw * 0.5f;
    const float cy = sh * 0.5f;
    const float dx = sx - cx;
    const float dy = sy - cy;
    outAngle = std::atan2f(dy, dx);
    const float absDx = std::fabsf(dx);
    const float absDy = std::fabsf(dy);
    const float halfW = sw * 0.5f - margin;
    const float halfH = sh * 0.5f - margin;
    if (halfW <= 1.f || halfH <= 1.f) return false;
    const float scale = std::min(halfW / (absDx + 1e-4f), halfH / (absDy + 1e-4f));
    outX = cx + dx * scale;
    outY = cy + dy * scale;
    return true;
}

bool TryCvtAnchor(const Vector3 &world, Object *battle, Object *minimap, bool bMiniMap, float &outX, float &outY) {
    if (!battle || !minimap) return false;
    Vector2 uv = bMiniMap ? CBattleSystem::CvtWorld2UISM(world) : CBattleSystem::CvtWorld2UIBM(world);
    Vector2 anchor = MinimapSys::GetFinalScreenPos(minimap, bMiniMap);
    Vector2 size = MinimapSys::GetFinalScreenSize(minimap, bMiniMap);
    if (size.x < 4.f || size.y < 4.f) return false;
    outX = anchor.x + uv.x * size.x;
    outY = anchor.y + uv.y * size.y;
    return true;
}

float MarginFor(MinimapSys::EMapType mode) {
    return mode == MinimapSys::EMapType::Big ? 48.f : 24.f;
}

const char *IconKeyForObj(const Snapshot &snap, unsigned int objId) {
    if (!objId) return nullptr;
    for (int i = 0; i < snap.iconCount && i < kMaxHeroIcons; ++i) {
        const IconItem &icon = snap.icons[i];
        if (icon.valid && icon.objId == objId && icon.key[0]) return icon.key;
    }
    return nullptr;
}

bool WorldToMinimap(const Vector3 &world, Object *minimap, float screenW, float screenH, float &outX, float &outY, MinimapSys::EMapType &mode) {
    outX = outY = 0.f;
    mode = MinimapSys::CurMapType(minimap);
    if (mode == MinimapSys::EMapType::None) return false;
    const bool bMiniMap = MinimapSys::IsMiniMapMode(mode);
    const float margin = MarginFor(mode);
    float sx = 0.f, sy = 0.f;
    if (MiniMapSysUT::WorldToScreenPoint(world, bMiniMap, screenH, sx, sy) && InRect(sx, sy, screenW, screenH, margin)) {
        outX = sx;
        outY = sy;
        return true;
    }
    if (TryCvtAnchor(world, CBattleSystem::GetInstance(), minimap, bMiniMap, sx, sy) && InRect(sx, sy, screenW, screenH, margin)) {
        outX = sx;
        outY = sy;
        return true;
    }
    if (MiniMapSysUT::WorldToScreenPoint(world, bMiniMap, screenH, sx, sy)) {
        outX = sx;
        outY = sy;
        return InRect(sx, sy, screenW, screenH, margin * 4.f);
    }
    return false;
}

void AddIcon(Snapshot &snap, int idx, unsigned int objId, const HeroIcon::Entry &entry) {
    if (idx < 0 || idx >= kMaxHeroIcons || !entry.fieldName) return;
    IconItem &item = snap.icons[idx];
    item.valid = true;
    item.objId = objId;
    CopyText(item.key, sizeof(item.key), entry.fieldName);
    EnsureIconPixels(entry.fieldName, entry);
}

void RefreshHeroIcons(Snapshot &snap, Object *kyriosMgr) {
    if (!kyriosMgr || !gLQConfig.esp.showHeroIcons) return;
    snap.iconCount = 0;
    const int linkerN = ActorManager::HeroCount(kyriosMgr);
    for (int i = 0; i < linkerN && snap.iconCount < kMaxHeroIcons; ++i) {
        Object *linker = ActorManager::HeroLinkerAt(i, kyriosMgr);
        uint32_t configId = 0, skinId = 0;
        if (!LinkerOk(linker) || !CBattleSystem::ReadLinkerMeta(linker, configId, skinId)) continue;

        const std::string displayName = CBattleSystem::GetHeroName(configId);
        const HeroIcon::Entry *entry = LookupEmbeddedIcon(displayName);
        if (!entry) continue;

        unsigned int objId = LinkerObjId(linker);
        if (!objId) objId = configId;
        AddIcon(snap, snap.iconCount++, objId, *entry);
    }
}

void BuildSnapshot(Snapshot &snap, Object *minimap, float dt, Object *gameActorMgr) {
    Snapshot prev{};
    {
        std::lock_guard<std::mutex> lock(gSnapMutex);
        prev = gBuffers[gActive.load(std::memory_order_acquire)];
    }
    const int prevIconCount = prev.iconCount;
    IconItem prevIcons[kMaxHeroIcons]{};
    if (prevIconCount > 0)
        std::memcpy(prevIcons, prev.icons, sizeof(prevIcons));

    snap = {};
    snap.screenW = static_cast<float>(GameViewport::width());
    snap.screenH = static_cast<float>(GameViewport::height());
    if (snap.screenW < 1.f || snap.screenH < 1.f) return;

    Frame frame{};
    if (!BuildFrame(frame, dt, gameActorMgr)) return;
    snap.valid = true;
    snap.hasMyWorld = frame.hasMyWorld;
    snap.targetCount = frame.targetCount;

    for (int i = 0; i < frame.targetCount && i < kMaxTargets; ++i) {
        const char *iconKey = IconKeyForObj(prev, frame.targetObjId[i]);
        const bool showWorldDistance = gLQConfig.esp.showDistance;
        const bool showHpPercent = true;
        const Vector3 *myWorld = frame.hasMyWorld ? &frame.myWorld : nullptr;
        const Vector3 *targetWorld = &frame.targetWorld[i];
        const float infoOffX = gLQConfig.esp.infoOffsetX;
        const float infoOffY = gLQConfig.esp.infoOffsetY;
        const float arrowMargin = gLQConfig.esp.arrowMargin > 0.f ? gLQConfig.esp.arrowMargin : 36.f;
        const bool hasTargetCd = gLQConfig.esp.showCooldowns && frame.targetCdValid[i];
        const LActorRoot::SkillCooldownInfo *targetCdPtr = hasTargetCd ? &frame.targetCd[i] : nullptr;
        bool targetOnScreen = TargetOnScreen(frame.targetWorld[i], snap.screenW, snap.screenH, 0.f);
        const bool needsInfoItem = NeedsInfoItem();
        if (frame.hasMyWorld && NeedsWorldOverlay()) {
            float a[2]{}, b[2]{};
            if (EspGUI::projectSegment(frame.myWorld, frame.targetWorld[i], snap.screenW, snap.screenH, a, b)) {
                targetOnScreen = InRect(b[0], b[1], snap.screenW, snap.screenH, 0.f);
                if (gLQConfig.esp.enabled) {
                    snap.lines[i].valid = true;
                    snap.lines[i].fromX = a[0];
                    snap.lines[i].fromY = a[1];
                    snap.lines[i].toX = b[0];
                    snap.lines[i].toY = b[1];
                }
                if (needsInfoItem && targetOnScreen && (frame.targetInfoValid[i] || hasTargetCd)) {
                    LActorRoot::ActorInfo info{};
                    if (frame.targetInfoValid[i]) info = frame.targetInfo[i];
                    SetInfoItem(snap, i, b[0] + infoOffX, b[1] + infoOffY, info, iconKey, showWorldDistance, showHpPercent, myWorld, targetWorld, targetCdPtr, gLQConfig.esp.showInfo);
                }
            }
        }

        if (gLQConfig.esp.showHeroIcons) {
            MinimapSys::EMapType mode = MinimapSys::EMapType::None;
            float mx = 0.f, my = 0.f;
            if (WorldToMinimap(frame.targetWorld[i], minimap, snap.screenW, snap.screenH, mx, my, mode)) {
                snap.mapItems[i].valid = true;
                snap.mapItems[i].objId = frame.targetObjId[i];
                snap.mapItems[i].x = mx;
                snap.mapItems[i].y = my;
                if (iconKey) CopyText(snap.mapItems[i].key, sizeof(snap.mapItems[i].key), iconKey);
            }
        }

        if (gLQConfig.esp.offscreenArrow && frame.hasMyWorld && !targetOnScreen) {
            float ax = 0.f, ay = 0.f, ang = 0.f;
            if (TryOffscreenArrow(frame.targetWorld[i], snap.screenW, snap.screenH, arrowMargin, ax, ay, ang)) {
                snap.arrows[i].valid = true;
                snap.arrows[i].x = ax;
                snap.arrows[i].y = ay;
                snap.arrows[i].angle = ang;
                if (needsInfoItem && (frame.targetInfoValid[i] || hasTargetCd)) {
                    LActorRoot::ActorInfo info{};
                    if (frame.targetInfoValid[i]) info = frame.targetInfo[i];
                    SetInfoItem(snap, i, ax + infoOffX, ay + infoOffY, info, iconKey, showWorldDistance, showHpPercent, myWorld, targetWorld, targetCdPtr, gLQConfig.esp.showInfo);
                }
            }
        }
    }

    if (gLQConfig.esp.showHeroIcons && prevIconCount > 0) {
        snap.iconCount = prevIconCount;
        std::memcpy(snap.icons, prevIcons, sizeof(snap.icons));
    }
}

void Publish(const Snapshot &snap) {
    std::lock_guard<std::mutex> lock(gSnapMutex);
    const int next = 1 - gActive.load(std::memory_order_relaxed);
    Snapshot copy = snap;
    copy.updatedMs = NowMs();
    gBuffers[next] = copy;
    gActive.store(next, std::memory_order_release);
}

} // namespace

void OnActorManagerUpdate(Object *kyriosMgr) {
    if (!il2cpp_loaded.load(std::memory_order_acquire)) return;
    if (!gLQConfig.esp.showHeroIcons) return;

    Snapshot snap{};
    if (!ReadSnapshot(snap) || !snap.valid) return;
    RefreshHeroIcons(snap, kyriosMgr);
    Publish(snap);
}

void OnGameUpdate(Object *gameActorMgr) {
    if (!il2cpp_loaded.load(std::memory_order_acquire)) return;
    const int heroN = gameActorMgr ? LGameActorMgr::HeroCount(gameActorMgr) : 0;
    if (!gLQConfig.esp.enabled && !gLQConfig.esp.showHeroIcons && !NeedsInfoItem() && !gLQConfig.esp.offscreenArrow) {
        Snapshot empty{};
        Publish(empty);
        return;
    }

    if (!gameActorMgr || heroN <= 0) {
        Snapshot empty{};
        Publish(empty);
        return;
    }

    Snapshot snap{};
    BuildSnapshot(snap, MinimapSys::Get(), SampleDeltaTime(), gameActorMgr);
    Publish(snap);
}

bool ReadSnapshot(Snapshot &out) {
    std::lock_guard<std::mutex> lock(gSnapMutex);
    out = gBuffers[gActive.load(std::memory_order_acquire)];
    if (!out.valid) return false;
    if (out.updatedMs > 0 && NowMs() - out.updatedMs > kSnapshotStaleMs) {
        out = {};
        return false;
    }
    return true;
}

bool GetIconPixels(const char *key, std::vector<uint8_t> &rgba, int &width, int &height, uint32_t &version) {
    rgba.clear();
    width = height = 0;
    version = 0;
    if (!key || !key[0]) return false;
    std::lock_guard<std::mutex> lock(gIconMutex);
    auto it = gIcons.find(key);
    if (it == gIcons.end() || !it->second.ready) return false;
    width = it->second.width;
    height = it->second.height;
    version = it->second.version;
    rgba = it->second.rgba;
    return true;
}

} // namespace EspRuntime
} // namespace lienquan
