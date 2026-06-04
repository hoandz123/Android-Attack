#include "EspRuntime.h"
#include "../Config/Config.h"
#include "../Data/HeroIcon/HeroIcon.h"
#include "SDK/ActorManager.h"
#include "SDK/CBattleSystem.h"
#include "SDK/KyriosFramework.h"
#include "SDK/LActorRoot.h"
#include "SDK/LGameActorMgr.h"
#include "SDK/LBattleLogic.h"
#include "SDK/MiniMapSysUT.h"
#include "SDK/MinimapSys.h"
#include <API/Il2CppApi.h>
#include <GameUI/EspGUI.h>
#include <GameUI/GameViewport.h>
#include <Includes/obfuscate.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstring>
#include <mutex>
#include <string>
#include <unordered_map>

#include <stb_image.h>

#define LOGGER_TAG "ATTACK_EspRuntime"
#include <Includes/Logger.h>

namespace lienquan {
namespace EspRuntime {
namespace {

constexpr float kStripSz = 44.f;
struct Frame {
    bool hasMyWorld = false;
    int myCamp = -1;
    int myEnemyCamp = -1;
    int targetCount = 0;
    unsigned int dbgHostPid = 0;
    unsigned int dbgMetaPid = 0;
    Vector3 myWorld{};
    Vector3 targetWorld[kMaxTargets]{};
    int targetCamp[kMaxTargets]{};
    unsigned int targetObjId[kMaxTargets]{};
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
uint32_t gSeq = 0;
int64_t gLastMs = 0;
int gUpdateLogCount = 0;

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
    const auto u = reinterpret_cast<uintptr_t>(o);
    return o && o->get_class() && (u & 3) == 0 && u > 0x10000;
}

unsigned int LinkerObjId(Object *linker) {
    if (!LinkerOk(linker)) return 0;
    if (linker->get_class()->find_method(OBF("get_ObjID"), 0))
        return linker->invoke_method<unsigned int>(OBF("get_ObjID"));
    return 0;
}

const HeroIcon::Entry *LookupEmbeddedIcon(const std::string &displayName) {
    if (displayName.empty()) return nullptr;
    return HeroIcon::FindByDisplayName(displayName.c_str());
}

bool DecodeEmbeddedIcon(const HeroIcon::Entry &entry, std::vector<uint8_t> &rgba, int &width, int &height) {
    rgba.clear();
    width = height = 0;
    if (!entry.iconBytes || entry.iconSize == 0) return false;
    int w = 0, h = 0, ch = 0;
    unsigned char *px =
        stbi_load_from_memory(entry.iconBytes, static_cast<int>(entry.iconSize), &w, &h, &ch, 4);
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

    static int loadLogCount = 0;
    if (loadLogCount < 24) {
        ++loadLogCount;
        LOGI(OBF("embedded icon ok key=%s name=%s %dx%d"),
             fieldKey, entry.displayName ? entry.displayName : "?", cache.width, cache.height);
    }
}

void CollectHero(Frame &f, float dt, Object *root, unsigned int hostPlayerId, int hostCamp,
                 int hostEnemyCamp, unsigned int hostObjId, bool myFromHost, unsigned int *seen, int &seenN) {
    if (!root) return;
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
    if (gLQConfig.esp.enemiesOnly && sameTeam) return;
    if (f.targetCount >= kMaxTargets) return;
    int camp = -1;
    if (teamKnown) camp = sameTeam ? hostCamp : hostEnemyCamp;
    f.targetCamp[f.targetCount] = camp;
    f.targetObjId[f.targetCount] = objId;
    f.targetWorld[f.targetCount++] = pos;
    if (objId && seenN < kMaxTargets) seen[seenN++] = objId;
}

bool BuildFrame(Frame &f, float dt, Object *gameActorMgr) {
    f = {};
    gSmoother.tick(dt);
    const int hostCamp = KyriosFramework::GetHostPlayerCamp();
    const unsigned int hostPlayerId = LBattleLogic::GetHostPlayerId();
    const int heroN = gameActorMgr ? LGameActorMgr::HeroCount(gameActorMgr) : 0;
    if (!gameActorMgr || heroN <= 0) return false;
    f.myCamp = hostCamp;
    f.dbgHostPid = hostPlayerId;

    unsigned int hostObjId = 0;
    int hostEnemyCamp = -1;
    bool myFromHost = false;
    Object *hostRoot = LBattleLogic::GetHostActorRoot();
    if (hostRoot) {
        hostObjId = LActorRoot::GetObjID(hostRoot);
        f.myWorld = gSmoother.get(gSmoother.keyFor(hostObjId), LActorRoot::GetWorldPosition(hostRoot), dt);
        f.hasMyWorld = true;
        hostEnemyCamp = LActorRoot::GetEnemyCamp(hostRoot);
        f.myEnemyCamp = hostEnemyCamp;
        f.dbgMetaPid = LActorRoot::GetActorPlayerId(hostRoot);
        myFromHost = true;
    }

    unsigned int seen[kMaxTargets]{};
    int seenN = 0;
    for (int i = 0; i < heroN; ++i) {
        CollectHero(f, dt, LActorRoot::HeroAt(i, gameActorMgr), hostPlayerId, hostCamp, hostEnemyCamp,
                    hostObjId, myFromHost, seen, seenN);
    }
    return f.hasMyWorld || f.targetCount > 0;
}

bool InRect(float x, float y, float sw, float sh, float margin) {
    return x >= -margin && y >= -margin && x <= sw + margin && y <= sh + margin;
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

bool WorldToMinimap(const Vector3 &world, Object *minimap, float screenW, float screenH,
                    float &outX, float &outY, MinimapSys::EMapType &mode) {
    outX = outY = 0.f;
    mode = MinimapSys::CurMapType(minimap);
    if (mode == MinimapSys::EMapType::None) return false;
    const bool bMiniMap = MinimapSys::IsMiniMapMode(mode);
    const float margin = MarginFor(mode);
    float sx = 0.f, sy = 0.f;
    if (MiniMapSysUT::WorldToScreenPoint(world, bMiniMap, screenH, sx, sy) &&
        InRect(sx, sy, screenW, screenH, margin)) {
        outX = sx;
        outY = sy;
        return true;
    }
    if (TryCvtAnchor(world, CBattleSystem::GetInstance(), minimap, bMiniMap, sx, sy) &&
        InRect(sx, sy, screenW, screenH, margin)) {
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
    if (idx < 0 || idx >= kMaxStripIcons || !entry.fieldName) return;
    IconItem &item = snap.icons[idx];
    item.valid = true;
    item.objId = objId;
    item.size = kStripSz;
    item.x = 12.f + idx * (kStripSz + 6.f) + kStripSz * .5f;
    item.y = snap.screenH - kStripSz - 12.f + kStripSz * .5f;
    CopyText(item.key, sizeof(item.key), entry.fieldName);
    EnsureIconPixels(entry.fieldName, entry);
}

void RefreshHeroIcons(Snapshot &snap, Object *kyriosMgr) {
    if (!kyriosMgr || !gLQConfig.esp.showHeroIcons) return;
    snap.iconCount = 0;
    const int linkerN = ActorManager::HeroCount(kyriosMgr);
    static int missLogCount = 0;
    for (int i = 0; i < linkerN && snap.iconCount < kMaxStripIcons; ++i) {
        Object *linker = ActorManager::HeroLinkerAt(i, kyriosMgr);
        uint32_t configId = 0, skinId = 0;
        if (!LinkerOk(linker) || !CBattleSystem::ReadLinkerMeta(linker, configId, skinId)) {
            if (missLogCount < 12) {
                ++missLogCount;
                LOGW(OBF("kyrios hero meta missing i=%d linker=%p"), i, linker);
            }
            continue;
        }

        const unsigned int objId = LinkerObjId(linker);
        if (!objId) continue;

        const std::string displayName = CBattleSystem::GetHeroName(configId);
        const HeroIcon::Entry *entry = LookupEmbeddedIcon(displayName);
        if (!entry) {
            if (missLogCount < 24) {
                ++missLogCount;
                LOGW(OBF("no embedded icon i=%d cfg=%u name='%s'"), i, configId,
                     displayName.empty() ? "?" : displayName.c_str());
            }
            continue;
        }

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
    IconItem prevIcons[kMaxStripIcons]{};
    if (prevIconCount > 0)
        std::memcpy(prevIcons, prev.icons, sizeof(prevIcons));

    snap = {};
    snap.seq = ++gSeq;
    snap.screenW = static_cast<float>(GameViewport::width());
    snap.screenH = static_cast<float>(GameViewport::height());
    if (snap.screenW < 1.f || snap.screenH < 1.f) return;

    Frame frame{};
    if (!BuildFrame(frame, dt, gameActorMgr)) return;
    snap.valid = true;
    snap.hasMyWorld = frame.hasMyWorld;
    snap.myCamp = frame.myCamp;
    snap.myEnemyCamp = frame.myEnemyCamp;
    snap.dbgHostPid = frame.dbgHostPid;
    snap.dbgMetaPid = frame.dbgMetaPid;
    snap.targetCount = frame.targetCount;
    snap.mapMode = MinimapSys::CurMapType(minimap);

    for (int i = 0; i < frame.targetCount && i < kMaxTargets; ++i) {
        snap.targetWorld[i] = frame.targetWorld[i];
        snap.targetCamp[i] = frame.targetCamp[i];
        snap.targetObjId[i] = frame.targetObjId[i];

        if (frame.hasMyWorld) {
            float a[2]{}, b[2]{};
            if (EspGUI::projectSegment(frame.myWorld, frame.targetWorld[i], snap.screenW, snap.screenH, a, b)) {
                snap.lines[i].valid = true;
                snap.lines[i].fromX = a[0];
                snap.lines[i].fromY = a[1];
                snap.lines[i].toX = b[0];
                snap.lines[i].toY = b[1];
            }
        }

        MinimapSys::EMapType mode = MinimapSys::EMapType::None;
        float mx = 0.f, my = 0.f;
        if (WorldToMinimap(frame.targetWorld[i], minimap, snap.screenW, snap.screenH, mx, my, mode)) {
            snap.mapItems[i].valid = true;
            snap.mapItems[i].objId = frame.targetObjId[i];
            snap.mapItems[i].camp = frame.targetCamp[i];
            snap.mapItems[i].x = mx;
            snap.mapItems[i].y = my;
            snap.mapMode = mode;
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
    gBuffers[next] = snap;
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
    if (!gLQConfig.esp.enabled && !gLQConfig.esp.minimapDot && !gLQConfig.esp.showHeroIcons &&
        !gLQConfig.esp.showDebug) {
        Snapshot empty{};
        Publish(empty);
        return;
    }

    Snapshot snap{};
    BuildSnapshot(snap, MinimapSys::Get(), SampleDeltaTime(), gameActorMgr);
    if (gUpdateLogCount < 20 && snap.seq % 30 == 1) {
        ++gUpdateLogCount;
        LOGI(OBF("update seq=%u valid=%d targets=%d icons=%d map=%d"),
             snap.seq, snap.valid ? 1 : 0, snap.targetCount, snap.iconCount, static_cast<int>(snap.mapMode));
    }
    Publish(snap);
}

bool ReadSnapshot(Snapshot &out) {
    std::lock_guard<std::mutex> lock(gSnapMutex);
    out = gBuffers[gActive.load(std::memory_order_acquire)];
    return out.valid;
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
