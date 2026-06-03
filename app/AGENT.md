# app — plugin host (`libattack.so`)

## Vai trò

Module **application** duy nhất. Build ra `libattack.so` (plugin thật) + APK tối thiểu. Java chỉ `loadLibrary("loader")`; logic nằm native + dex embed.

## Phụ thuộc Gradle

| Module | Mục đích |
|--------|----------|
| `:loader` | `libloader.so` trong APK, `dlopen` plugin |
| `:native-core` | Prefab: gameapi, tools, kitty, dobby, dexloader, activitytracker, imgui, curl, httpclient, … |
| `:mod-ui` | Prefab: modui (menu shell + GLES) |
| `:native-dex` | **Không** `implementation` — chỉ task `generateEmbeddedDex` |

## Native (`src/main/cpp/`)

| File / thư mục | Việc |
|------|------|
| `NativeLib.cpp` | `JNI_OnLoad`: dex → tracker → `modui::Init()` → `games::Dispatch(pkg)` |
| `Games.{hpp,cpp}` | Registry đa game: `REGISTER_GAME`, `Dispatch(package)` |
| `Menu.{hpp,cpp}` | `appui::RegisterMenu()` — menu **fallback** khi package không khớp game nào |
| `PlayTogether/` | Game Play Together (đầy đủ) — xem `PlayTogether/AGENT.md` |
| `LienQuan/` | Game Liên Quân (stub) — xem `LienQuan/AGENT.md` |
| `CMakeLists.txt` | Link `attack` SHARED; gộp source mọi game; include `EMBEDDED_DEX_DIR` |

### Thứ tự `JNI_OnLoad` (giữ nguyên thứ tự)

1. `jni::Init(vm)`
2. `dex_loader::Init(vm, embedded_dex::data, size)`
3. `activity_tracker::Init(vm)` → Java `ActivityTrackerBridge.install()` (+ `Surface` overlay)
4. `modui::Init()`
5. `games::Dispatch(pkg)` → game khớp gọi `Activate()`; **không khớp** → `appui::RegisterMenu()` (fallback)

## Hệ thống đa game (`Games.hpp` / `Games.cpp`)

Mỗi game tự đăng ký lúc nạp `.so` (static init):

```cpp
REGISTER_GAME(OBF("com.vng.playtogether"), Activate);
```

`games::Dispatch(package)` so `package` (lấy từ `Tools::GetPackageName()`) với registry; khớp thì gọi `Activate()` của game đó và trả `true`. Một APK chứa mọi game; chỉ game khớp package process hiện tại được kích hoạt.

### Thêm game mới

1. Tạo thư mục `src/main/cpp/<Game>/` (theo mẫu `PlayTogether/`: `Config/`, `Hook/`, `UI/`, `SDK/`).
2. Viết `<Game>.cpp`: hàm `Activate()` (dựng `modui::AppUi` + cài hook) + `REGISTER_GAME("<package>", Activate)`.
3. Thêm source vào `CMakeLists.txt` (`add_library(attack ...)`).
4. Tạo `<Game>/AGENT.md` bao quát game (bắt buộc — mỗi game 1 AGENT.md).
5. Build `./gradlew :app:assembleDebug`.

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

- `ATTACK_PlayTogether` — plugin init (`NativeLib.cpp`) + game (tag theo từng game, xem AGENT.md của game)
- `DexLoader`, `ActivityTracker` — từ native-core / native-dex
- `AttackLoader` — từ `:loader`

## Khi sửa

- Logic/menu một game cụ thể → thư mục game đó (`PlayTogether/`, `LienQuan/`), **không** sửa `Menu.cpp`.
- Menu fallback (không thuộc game nào) → `Menu.cpp`.
- Thêm game / đổi dispatch → `Games.cpp` + `CMakeLists.txt` (xem "Thêm game mới").
- Thứ tự khởi tạo / thêm init chung → `NativeLib.cpp`.
- Không nhét Java bridge vào app; dex Java thuộc `:native-dex`.
