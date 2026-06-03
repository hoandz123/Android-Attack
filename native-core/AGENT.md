# native-core — thư viện native dùng chung (Prefab)

## Vai trò

Android library **chỉ C++**. Xuất static libs qua **Prefab** cho `:app` và `:mod-ui`. Không có Java (dex Java nằm `:native-dex`).

## Prefab modules

| Module | Nội dung |
|--------|----------|
| `lgl` | Headers LGL (`Includes/`, Logger, obfuscate) |
| `dobby` | Hook (prebuilt `.a` per ABI) |
| `kitty` | Memory patch / backup |
| `imgui` | ImGui + Android/GLES3 backend |
| `curl` | libcurl static per ABI |
| `httpclient` | `http::Get` / `Post` / `Download(url, path)` (curl) |
| `filemanager` | `fs::` đọc/ghi, Mkdir, ListDir, CopyFile, RenamePath, Remove (`FileManager/`) |
| `jnihelper` | `JavaVM` + `JNIEnv` thống nhất (`JNIHelper/`) |
| `dexloader` | In-memory DEX inject (`DexLoader/`) |
| `activitytracker` | JVM activity refs + JNI bridge |
| `gameapi` | **IL2CPP runtime API** + System/UnityEngine wrappers (`API/`): `FindClass`, `GET_METHOD`, `Object`/`List`/`Dictionary`, `Init_Il2cpp_Symbol` |
| `tools` | Hook/memory helper (`Tools/`): `Tools::Hook` (A64Hook arm64 / Dobby), maps, `GetPackageName`, `Sleep`, `HEX_*` |
| `xdl` | `dlopen`/`dlsym`/`dladdr` bypass linker namespace (`xdl/`, C) |
| `base64` | Encode/decode + HMG helpers (`Base64/`) |
| `sharedprefs` | Key/value lưu file (`SharedPrefs/`) |
| `gameui` | ESP draw + viewport size (`GameUI/`, dùng imgui + gameapi) |

Headers: `src/main/cpp/` (lgl/jnihelper/dexloader/activitytracker/gameapi export cả cpp root).

**Static lib nội bộ (không publish Prefab):** `json` (nlohmann INTERFACE), `xhook` (PLT/GOT hook), `antilibpatch` (chống vá `.text`), `apksig` (verify chữ ký APK — cần header OpenSSL/BoringSSL mới compile).

## Thư mục chính (`src/main/cpp/`)

```
API/                IL2CPP API (Il2CppApi.h, Il2cpp_Struct.h, Il2cpp_Symbol.h) + API/game/{System,UnityEngine}
Tools/              Tools::Hook, GetPackageName, maps, sleep (+ And64Hook arm64)
FileManager/        fs::Exists, ReadBytes, WriteBytes, MkdirP, ListDir, …
JNIHelper/          jni::Init, Env(), ScopedEnv, FindClass, RegisterNatives
DexLoader/          DexLoader.cpp, JniReflect — makeInMemoryDexElements
ActivityTracker/    Init → Java install; nativeOn* giữ global ref Activity
GameUI/             EspGUI, GameViewport (vẽ ESP)
SharedPrefs/        key/value lưu file
Base64/             encode/decode
xdl/ xhook/         dlopen bypass / PLT-GOT hook (C)
imgui/              vendored
kittymemory/        KittyMemory
dobby/{abi}/        libdobby.a
curl/{abi}/         libcurl_all.a
cmake/PrefabStatic.cmake   link .a từ prefab (INTERFACE rỗng)
```

## JNIHelper — API

```cpp
jni::Init(vm);              // JNI_OnLoad, một lần
JNIEnv *e = jni::Env();     // thread đã attach — không Detach
jni::ScopedEnv scoped;      // native thread chưa attach → Attach, destructor Detach
jclass c = jni::FindClass(e, "com/android/attack/nativedex/EglOverlay");
jni::RegisterNatives(e, slash_class, methods, count);
```

**Không** gọi `DetachCurrentThread` trên thread đang chạy JNI callback (render thread, v.v.).

## FileManager — API (`#include <FileManager.hpp>`, namespace `fs`)

```cpp
bool Exists(path);
bool IsFile(path); bool IsDir(path);
int64_t FileSize(path);
Result Remove(path); Result RenamePath(from, to); Result CopyFile(src, dst);
Result Mkdir(path); Result MkdirP(path);
std::vector<std::string> ListDir(dir);
Result WriteBytes(path, data, len); Result AppendBytes(path, data, len);
std::vector<uint8_t> ReadBytes(path, Result *out = nullptr);
std::string Join(a, b); Dirname(path); Basename(path);
```

Consumer CMake: `native-core::filemanager`

## DexLoader — API

```cpp
bool Init(JavaVM *vm, const uint8_t *dex, size_t size);
bool LoadIntoContext(JNIEnv *env, jobject context, ...);
```

**Hành vi `Init`:**

- `SDK_INT < 26` → fail (cần `makeInMemoryDexElements`)
- Retry `currentApplication` (30×20ms)
- API 29+: log ClassLoader chain; duyệt `getParent()` tìm `pathList`
- Verify `com/android/attack/nativedex/ActivityTrackerBridge` sau inject

## ActivityTracker — API

```cpp
bool Init(JavaVM *vm);  // jni::Init + RegisterNatives + Java install()
jobject CurrentActivity(JNIEnv *env);
std::vector<jobject> GetActivities(JNIEnv *env);
```

Java lifecycle nằm `ActivityTrackerBridge` (`:native-dex`).

## Build

```bash
./gradlew :native-core:assembleDebug
```

Prefab package: `build/intermediates/prefab_package/release/prefab/`

**Sau `:native-core:clean`:** cần build lại native-core trước app (Prefab mất).

Consumer CMake: `-DNATIVE_CORE_PREFAB=.../prefab` + `link_prefab_static` + `link_native_core_prebuilts`.

## Tương thích API

| API | DexLoader | ActivityTracker |
|-----|-----------|-----------------|
| 26–28 | OK | OK (sync `mActivities` reflection) |
| 29+ | OK + parent ClassLoader | OK; sync có thể 0 trên ROM lạ |
| 24–25 | **Không** | Chạy được nếu dex đã load bằng cách khác |

## Logcat

`DexLoader`, `JniReflect`, `ActivityTracker`

## Khi sửa

- API IL2CPP / wrapper class game chung → `API/` (`gameapi`); `Tools::Hook`, maps → `Tools/` (`tools`)
- Hook/patch/memory → kitty/dobby
- Cơ chế inject dex → `DexLoader/` (không trộn Java)
- Tracker state native → `ActivityTracker.cpp`; đăng ký lifecycle → sửa Java ở `:native-dex`
- ESP draw / viewport → `GameUI/` (`gameui`)

## Không làm ở đây

- Embed dex bytes, d8, `embedded_dex.hpp`
- UI menu shell (thuộc `:mod-ui`)
- `libattack.so` final link (thuộc `:app`)
