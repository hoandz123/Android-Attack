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
| `jnihelper` | `JavaVM` + `JNIEnv` thống nhất (`JNIHelper/`) |
| `dexloader` | In-memory DEX inject (`DexLoader/`) |
| `activitytracker` | JVM activity refs + JNI bridge |

Headers: `src/main/cpp/` (dexloader/activitytracker export cả cpp root).

## Thư mục chính (`src/main/cpp/`)

```
JNIHelper/          jni::init, env(), ScopedEnv, find_class, register_natives
DexLoader/          DexLoader.cpp, JniReflect — makeInMemoryDexElements
ActivityTracker/    init → Java install; nativeOn* giữ global ref Activity
imgui/              vendored
kittymemory/        KittyMemory
dobby/{abi}/        libdobby.a
curl/{abi}/         libcurl_all.a
cmake/PrefabStatic.cmake   link .a từ prefab (INTERFACE rỗng)
```

## JNIHelper — API

```cpp
jni::init(vm);              // JNI_OnLoad, một lần
JNIEnv *e = jni::env();     // thread đã attach — không Detach
jni::ScopedEnv scoped;      // native thread chưa attach → Attach, destructor Detach
jclass c = jni::find_class(e, "com/android/attack/nativedex/EglOverlay");
jni::register_natives(e, slash_class, methods, count);
```

**Không** gọi `DetachCurrentThread` trên thread đang chạy JNI callback (render thread, v.v.).

## DexLoader — API

```cpp
bool init(JavaVM *vm, const uint8_t *dex, size_t size);
bool load_into_context(JNIEnv *env, jobject context, ...);
```

**Hành vi `init`:**

- `SDK_INT < 26` → fail (cần `makeInMemoryDexElements`)
- Retry `currentApplication` (30×20ms)
- API 29+: log ClassLoader chain; duyệt `getParent()` tìm `pathList`
- Verify `com/android/attack/nativedex/ActivityTrackerBridge` sau inject

## ActivityTracker — API

```cpp
bool init(JavaVM *vm);  // jni::init + RegisterNatives + Java install()
jobject current_activity(JNIEnv *env);
std::vector<jobject> activities(JNIEnv *env);
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

- Hook/patch/memory → kitty/dobby
- Cơ chế inject dex → `DexLoader/` (không trộn Java)
- Tracker state native → `ActivityTracker.cpp`; đăng ký lifecycle → sửa Java ở `:native-dex`

## Không làm ở đây

- Embed dex bytes, d8, `embedded_dex.hpp`
- UI menu shell (thuộc `:mod-ui`)
- `libattack.so` final link (thuộc `:app`)
