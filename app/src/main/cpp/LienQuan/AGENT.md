# LienQuan — game mod (Liên Quân Mobile)



**Nhánh:** `LienQuan`. Package: `com.garena.game.kgvn`. Dump: `dump/com.garena.game.kgvn/com.garena.game.kgvn_1.62.1.6_[arm64-v8a].cs`.



## SDK (từ dump)



| Wrapper | IL2CPP class | Ghi chú |

|---------|--------------|---------|

| `KyriosFramework` | `Kyrios.KyriosFramework` | `MonoSingleton` — battle core, `GetInstance`, `get_IsRunning`, `get_actorManager` |

| `CLobbySystem` | `Assets.Scripts.GameSystem.CLobbySystem` | `Singleton` — lobby, `GetInstance` |

| `KyriosActorManager` | `Kyrios.Actor.ActorManager` | Qua `KyriosFramework::get_actorManager`, field `HeroActors` |



## Map sáng (`main.mapHack`)



- UI: checkbox **Map sáng** (`Chinh.cpp`).

- **Một hook logic** (`Project.Plugins_d.dll`): `LVActorLinker.SetVisible(camp, bVisible, forceSync)` — khi mapHack, chặn `bVisible=false` nếu `camp == hostPlayerCamp` (`getHostCamp` qua `KyriosFramework.get_hostLogic`).



## Anti-cheat bypass (`Bypass/AntiCheat::init`)

- `Hook/Bypass/AntiCheat.cpp` — `il2cpp_bypass::install()` + thread `anogs_bypass::apply_anogs_patches()`.
- `Hook/Bypass/il2cpp_bypass.hpp` — hook `AnoSDK` / `TssSdkCom` / `LSynchrReport` / `LStateSynchr` (stub `false`); egress: `NetworkModule.SendLobbyMsg` (chặn `0xBB8`/`0xBEA`/`0x32AA`), `LNetwork.SendGameMsg` (chặn `0x507` RELAYHASHCHECK).
- `Hook/Bypass/anogs_bypass.hpp` — memory patch `libanogs.so` (ret/nop/mov) tại các offset cố định; chờ lib tối đa 120s qua xdl.
- Báo cáo phân tích native (tham khảo): `dump/com.garena.game.kgvn/libanogs-deep-analysis.md`.



## Entry & luồng



1. Menu tab **Chính** (`UI/Tab/Chinh.cpp`).

2. Thread: `Init_Il2cpp_Symbol()` → `Il2CppDomain::dump_domain()` (lần đầu) → `Hook::init()` (`LoadConfig`, `AntiCheat::init`, `MapBright::InstallHooks`).



## Cấu trúc



```

LienQuan/

  LienQuan.cpp

  Config/

  Hook/Hook.cpp, Hook/Bypass/*, MapBright.*

  Hook/SDK/KyriosFramework.*, CLobbySystem.*, KyriosActorManager.*

  UI/Tab/Chinh.*

```



Include: tương đối theo file — không `-I LienQuan/` global trong CMake.

