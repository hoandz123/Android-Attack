# native-dex — Java embed (không vào APK merge)

## Vai trò

Android library **chỉ Java** (`src/main/java/`). Toàn bộ class biên dịch → **một** `classes.dex` → nhúng vào `libattack.so` qua `embedded_dex.hpp`. Runtime: `dex_loader::Init` inject vào `Application` ClassLoader.

**`:app` không `implementation(project(":native-dex"))`** — APK không chứa các class này trong dex merge chính.

## Package

`com.android.attack.nativedex`

Class chính: `ActivityTrackerBridge` — lifecycle + `install()` + native `nativeOn*`.

`EglOverlay` — EGL overlay (chỉ vẽ). `TouchInputBridge` — `Window.Callback` proxy → touch + key. `KeyboardInputBridge` — IME sink + composition → native.

## Tương thích API (minSdk **26**)

| Thành phần | API dùng | Ghi chú |
|------------|----------|---------|
| DexLoader | 26+ | `makeInMemoryDexElements` |
| `Surface.isValid()` | 26+ | luôn gọi (minSdk 26) |
| `setOnApplyWindowInsetsListener` | 21+ | luôn gọi |
| Cutout insets | 28+ (P) | nhánh riêng trong `TouchInputBridge` |
| `WindowInsets.Type.systemBars()` | 30+ (R) | fallback `getSystemWindowInset*` 26–29 |
| `MotionEvent.getRawX(index)` | 29+ (Q) | fallback `getRawX()` 26–28 |
| `KeyboardInputBridge` | 11+ IME APIs | `commitText` / `setComposingText` / `deleteSurroundingText` |
| `IMPORTANT_FOR_ACCESSIBILITY_NO` | 16+ | guard JELLY_BEAN (redundant trên 26) |

## Thêm class mới

1. Tạo `.java` dưới `src/main/java/...`
2. `./gradlew :app:assembleDebug` (tự `generateEmbeddedDex`)
3. Gọi từ native (`FindClass` / `CallStatic*`) hoặc từ Java khác trong cùng module

**Không** cần khai báo từng file trong Gradle. Cả module → một `classes.dex`.

**Cảnh báo:** method **chỉ** gọi từ JNI — không dùng `d8 --release` (đã tắt trong embed script) để tránh bị strip.

## Pipeline embed

```
compile → syncDebugLibJars → classes.jar
  → generateEmbeddedDex (d8, không --release)
  → native-dex/build/generated/embed/
       classes.dex
       embedded_dex.hpp   // namespace embedded_dex { data[], size }
  → :app link libattack.so (#include <embedded_dex.hpp>)
```

Script: `gradle/embed-native-dex.gradle.kts` (apply trong `native-dex/build.gradle.kts`).

- `syncDebugLibJars` → `finalizedBy(generateEmbeddedDex)`
- `generateEmbeddedDex`: `outputs.upToDateWhen { false }` — luôn chạy lại khi có trong graph build
- `:app` `dependsOn(:native-dex:generateEmbeddedDex)` trước CMake

## `ActivityTrackerBridge.install()`

- Retry `currentApplication` (Java 30×20ms; native dex init cũng retry)
- `registerActivityLifecycleCallbacks`
- `syncExistingActivities()` — reflection `ActivityThread.mActivities` (try/catch, có thể 0 activity trên API cao)
- Lifecycle → `nativeOnResumed/Paused/Destroyed` (C++ global ref)

Gọi từ `activity_tracker::Init` sau `RegisterNatives`.

## Build

```bash
./gradlew :native-dex:generateEmbeddedDex   # chỉ embed
./gradlew :app:assembleDebug                 # full
```

## Output copy (root)

`out/dex/{variant}/native-dex/classes.dex` — sau `assembleDebug` app (script root `copy-build-outputs.gradle.kts`).

## Hidden API

Dùng reflection `android.app.ActivityThread` — không import trực tiếp (SDK hidden). Lỗi ghi log, không crash process.

## Logcat

Tag `ActivityTracker`

## Giới hạn

- Một file DEX embed; module lớn (>64K methods) cần mở rộng pipeline
- Class mới **không tự chạy** — cần entry (native hoặc gọi `install()` tương tự)
- DexLoader tối thiểu **API 26** (native-core)

## Khi sửa

- Bridge / helper Java an toàn (try/catch) → đây
- Logic native ref Activity → `:native-core/ActivityTracker/`
- Cơ chế inject → `:native-core/DexLoader/`

## File phụ

`proguard-embed.pro` — tham chiếu keep (d8 CLI hiện không dùng `--pg-conf`)
