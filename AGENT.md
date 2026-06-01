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
       → modui + app_menu
```

## Build nhanh

```bash
./gradlew :app:assembleDebug
```

→ `out/dex/debug/`, `out/lib/{attack,loader}/debug/{abi}/`

## Gradle root

- `gradle/copy-build-outputs.gradle.kts` — copy sau assemble
- `gradle/embed-native-dex.gradle.kts` — apply từ `:native-dex` only

## SDK dự án

`minSdk 26`, `target/compile 36` — khớp Dex in-memory (`DexLoader`).

## Clean

`./gradlew clean` trên app xóa `out/`. Sau clean native-core: build lại native-core rồi app.
