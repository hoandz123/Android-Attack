# LienQuan — game mod (Liên Quân Mobile)

**Nhánh:** `LienQuan`. Package: `com.garena.game.kgvn`. Dump: `dump/com.garena.game.kgvn/com.garena.game.kgvn_1.62.1.6_[arm64-v8a].cs`.

## SDK

| Module | IL2CPP | Vai trò |
|--------|--------|---------|
| `KyriosFramework` | `Kyrios.KyriosFramework`, `VHostLogic` | `get_IsRunning`, `view_hero_count` |
| `LBattleLogic` | `LLogicCore`, `LFramework`, `LBattleLogic` | `get` = main → desk, `get_gameActorMgr` |
| `LGameActorMgr` | `LGameActorMgr.HeroActors` | `hero_count`, `GetActor`, `GetCampHeroActors` |
| `LActorRoot` | `LActorRoot`, `VInt3` | `get_world_position`, `hero_at`, camp/objId |

ESP: `LBattleLogic` → `LGameActorMgr` → `LActorRoot` pos logic.

## Map sáng (`main.mapHack`)

- `LVActorLinker.SetVisible` + `KyriosFramework::get_hostPlayerCamp()`.

## ESP line (`esp.enabled`)

- `UI/Tab/Esp.cpp` — getter SDK mỗi frame, không snapshot.

## ESP minimap (`esp.minimapDot`)

- `Hook/EspHelper` — `Logic::Build` (LActorRoot), `Minimap::WorldToScreen` (SDK minimap), `Draw::Overlay`.
- Vị trí world từ `LActorRoot` (cùng pipeline ESP line).

## Anti-cheat bypass

`Hook/Bypass/*` — xem `libanogs-deep-analysis.md`.

## Cấu trúc

```
LienQuan/
  Hook/SDK/KyriosFramework.cpp, LBattleLogic.cpp, LGameActorMgr.cpp, LActorRoot.cpp
  Hook/Hook.cpp, Hook/EspHelper.cpp, Bypass/*
  Config/, UI/Tab/Esp.cpp (UI only)
```
