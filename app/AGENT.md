# app — plugin host (`libattack.so`)

## Vai trò

Module **application** duy nhất. Build ra `libattack.so` (plugin thật) + APK tối thiểu. Java chỉ `loadLibrary("loader")`; logic nằm native + dex embed.

## Phụ thuộc Gradle

| Module | Mục đích |
|--------|----------|
| `:loader` | `libloader.so` trong APK, `dlopen` plugin |
| `:native-core` | Prefab: dexloader, activitytracker, imgui, kitty, dobby, curl |
| `:mod-ui` | Prefab: modui (menu shell + GLES) |
| `:native-dex` | **Không** `implementation` — chỉ task `generateEmbeddedDex` |

## Native (`src/main/cpp/`)

| File | Việc |
|------|------|
| `NativeLib.cpp` | `JNI_OnLoad`: dex → tracker → `modui::Init()` → menu app |
| `Menu.cpp` | `set_window_title`, `add_tab` |
| `CMakeLists.txt` | Link `attack` SHARED; include `EMBEDDED_DEX_DIR` |

### Thứ tự `JNI_OnLoad` (giữ nguyên thứ tự)

1. `dex_loader::Init(vm, embedded_dex::data, size)`
2. `activity_tracker::Init(vm)` → Java `ActivityTrackerBridge.install()` (+ `Surface` overlay)
3. `modui::Init()` → `appui::RegisterMenu()`

## CMake args (tự set trong `build.gradle.kts`)

- `NATIVE_CORE_CPP`, `NATIVE_CORE_PREFAB`, `MOD_UI_PREFAB`
- `EMBEDDED_DEX_DIR` → `native-dex/build/generated/embed` (chứa `embedded_dex.hpp`)

**Lưu ý:** Build `:native-core` / `:mod-ui` trước lần đầu (Prefab). Đường dẫn dùng `/` (Windows đã replace).

## Build

```bash
./gradlew :app:assembleDebug
```

- `preBuild` + `configureCMake*` phụ thuộc `:native-dex:generateEmbeddedDex`
- `clean` xóa `app/.cxx`, `out/`
- `externalNativeBuildClean*` **tắt** (tránh lỗi Prefab sau clean native-core)

## Output

| Artifact | Vị trí |
|----------|--------|
| APK | `app/build/outputs/apk/` |
| `libattack.so` | merge native libs trong APK + `out/lib/attack/{variant}/{abi}/` |

## Java

`MainActivity`: `static { System.loadLibrary("loader"); }` — **không** thêm JNI ở đây.

## Logcat

- `AttackPlugin` — plugin init
- `DexLoader`, `ActivityTracker` — từ native-core / native-dex

## Khi sửa

- Menu UI app → `Menu.cpp`
- Thứ tự khởi tạo / thêm init → `NativeLib.cpp`
- Không nhét Java bridge vào app; dex Java thuộc `:native-dex`
