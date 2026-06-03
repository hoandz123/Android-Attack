# loader — bootstrap (`libloader.so`)

## Vai trò

Nạp plugin `libattack.so` theo **2 chế độ**:

1. **In-APK** (`Loader.cpp`): APK gọi `System.loadLibrary("loader")` → `JNI_OnLoad` tìm `nativeLibraryDir` → `dlopen` + gọi `JNI_OnLoad` plugin. (Đường chính trên thiết bị/ARM.)
2. **Zygisk module** (`Zygisk/zygisk.cpp`): khi `libloader.so` cài như module Magisk/Zygisk — dùng cho **emulator x86/x86_64**: nạp payload ARM qua **Native Bridge** + hook chống phát hiện.

## Plugin mặc định

```cpp
static const char *kPlugins[] = {"libattack.so"};
```

Thêm SO: sửa mảng trong `Loader.cpp`, đảm bảo file nằm cùng thư mục native lib.

## Cấu trúc

```
loader/src/main/cpp/
  Loader.cpp           # in-APK: JNI_OnLoad → dlopen plugin
  Zygisk/zygisk.cpp    # Zygisk module (MyModule) + native bridge + hook access()
  Zygisk/zygisk.hpp    # Zygisk module API
  CMakeLists.txt       # target loader SHARED (gộp xdl/dobby/Includes từ native-core)
```

## Luồng — In-APK (`Loader.cpp`)

```
System.loadLibrary("loader")
  → JNI_OnLoad
  → nativeLibraryDir (ActivityThread.currentApplication → ApplicationInfo)
  → dlopen(basename "libattack.so", RTLD_NOW|RTLD_GLOBAL)  (fallback: dir + "/libattack.so")
  → dlsym("JNI_OnLoad") → plugin JNI_OnLoad
```

## Luồng — Zygisk (`Zygisk/zygisk.cpp`, build x86/x86_64)

```
REGISTER_ZYGISK_MODULE(MyModule)   // chỉ compile ở x86/x86_64 (#else guard)
  preAppSpecialize  → lọc package (com.vng.playtogether, com.haegin.playtogether); khác → DLCLOSE
  postAppSpecialize → thread hack_prepare:
       1) hook libc access() (xdl_sym + DobbyHook) → chặn path phát hiện
          emulator/root/magisk (g_accessBlockList)
       2) mmap <app_data_dir>/libattack.so (ARM) → load qua Native Bridge
          (libhoudini / native.bridge, loadLibraryExt + memfd) → JNI_OnLoad payload
```

> ARM build: `postAppSpecialize` gọi thẳng `JNI_OnLoad` (không `REGISTER_ZYGISK_MODULE`).

## Build

```bash
./gradlew :loader:assembleDebug
```

Output: `libloader.so` → `out/lib/loader/{variant}/{abi}/` (sau `assembleDebug` app).

## Phụ thuộc

- **Không** Gradle `:native-core` / Prefab, nhưng CMake `-DNATIVE_CORE_CPP=...` để **biên dịch trực tiếp** source native-core: `xdl/*.c`, `dobby/DobbyLink.cpp`, `Includes/oxorany.cpp`, `IncludesLink.cpp`; link `libdobby.a` per ABI.
- ABI: arm64-v8a, armeabi-v7a, x86, x86_64 (đồng bộ app).

## Logcat

- `ATTACK_Loader` — `Loader.cpp` (`JNI_OnLoad`, `dlopen`/`dlsym`).
- `ATTACK_LoaderZygisk` — Zygisk (detect game, native bridge, hook `access`).

## Lưu ý

- Plugin phải export `JNI_OnLoad` trả `JNI_VERSION_1_6`.
- `RTLD_GLOBAL` để symbol plugin dùng chung với loader/process.
- In-APK thử **basename** trước (`extractNativeLibs=false` → file có thể không trên disk), rồi `nativeLibraryDir/...`.
- `:app` `packaging.jniLibs.useLegacyPackaging = true` — extract `.so` ra `nativeLibraryDir`.
- Zygisk x86: payload là `libattack.so` **ARM** trong data dir game, chạy qua native bridge; bỏ qua nếu `lib_dir` chứa `/lib/x86`.

## Khi sửa

- Đổi tên plugin / multi-plugin → `kPlugins` trong `Loader.cpp`.
- Target package / chống phát hiện / native bridge → `Zygisk/zygisk.cpp` (`g_accessBlockList`, `preSpecialize`).
- **Không** nhét dexloader/tracker vào loader — thuộc `libattack.so`.
