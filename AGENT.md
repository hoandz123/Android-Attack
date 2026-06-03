# AndroidAttack — monorepo map

## Modules

| Module | Artifact | AGENT.md |
|--------|----------|----------|
| `app` | APK + `libattack.so` | [app/AGENT.md](app/AGENT.md) |
| `loader` | `libloader.so` | [loader/AGENT.md](loader/AGENT.md) |
| `native-core` | Prefab static libs | [native-core/AGENT.md](native-core/AGENT.md) |
| `mod-ui` | Prefab `modui` | [mod-ui/AGENT.md](mod-ui/AGENT.md) |
| `native-dex` | embed dex (không APK dex) | [native-dex/AGENT.md](native-dex/AGENT.md) |

## Luồng tổng

```
APK load libloader.so
  → dlopen libattack.so
       → dex_loader::Init (embedded_dex.hpp từ :native-dex)
       → activity_tracker::Init
       → modui::Init
       → games::Dispatch(package)         (Games.cpp)
            ├─ khớp → <game>::Activate()   (menu + hook game đó)
            └─ không khớp → appui::RegisterMenu()  (Menu.cpp, fallback)
```

## Games (đa game trong 1 APK)

Mỗi game tự đăng ký qua `REGISTER_GAME(package, Activate)`; `Dispatch` chọn theo package process. **Mỗi game 1 AGENT.md riêng:**

| Game | Package | AGENT.md |
|------|---------|----------|
| Play Together | `com.vng.playtogether` | [app/src/main/cpp/PlayTogether/AGENT.md](app/src/main/cpp/PlayTogether/AGENT.md) |
| Liên Quân (stub) | `com.garena.game.kgvn` | [app/src/main/cpp/LienQuan/AGENT.md](app/src/main/cpp/LienQuan/AGENT.md) |

Thêm game mới: xem [app/AGENT.md](app/AGENT.md) §"Thêm game mới".

## Build nhanh

```bash
./gradlew :app:assembleDebug
```

→ `out/dex/debug/`, `out/lib/{attack,loader}/debug/{abi}/`

## Gradle root

- `gradle/copy-build-outputs.gradle.kts` — copy sau assemble
- `gradle/zygisk-deploy.gradle.kts` — adb push `libattack.so`; package: `attack.deployTargetPackage` trong `gradle.properties`
- `gradle/embed-native-dex.gradle.kts` — apply từ `:native-dex` only

## SDK dự án

`minSdk 26`, `target/compile 36` — khớp Dex in-memory (`DexLoader`).

## Dump IL2CPP

Theo **package** — xem [dump/README.md](dump/README.md):

- Play Together (VNG): `dump/com.vng.playtogether/` (`*.cs` + `libil2cpp.so` cùng version)
- Global PT: `dump/com.haegin.playtogether/`
- Liên Quân: `dump/com.garena.game.kgvn/`

## Clean

`./gradlew clean` trên app xóa `out/`. Sau clean native-core: build lại native-core rồi app.
