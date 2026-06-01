# loader — bootstrap (`libloader.so`)

## Vai trò

Library native **duy nhất** được APK load trực tiếp (`System.loadLibrary("loader")`). `JNI_OnLoad` tìm `nativeLibraryDir`, `dlopen` + gọi `JNI_OnLoad` của plugin.

## Plugin mặc định

```cpp
static const char *kPlugins[] = {"libattack.so"};
```

Thêm SO: sửa mảng trong `loader/src/main/cpp/loader.cpp`, đảm bảo file nằm cùng thư mục native lib trong APK.

## Luồng runtime

```
loadLibrary("loader")
  → JNI_OnLoad (loader)
  → nativeLibraryDir + "/libattack.so"
  → dlopen(RTLD_NOW | RTLD_GLOBAL)
  → dlsym("JNI_OnLoad") → plugin JNI_OnLoad
```

## Cấu trúc

```
loader/src/main/cpp/
  loader.cpp      # toàn bộ logic
  CMakeLists.txt  # target loader SHARED
```

Không Java logic (chỉ manifest nếu có).

## Build

```bash
./gradlew :loader:assembleDebug
```

Output: `libloader.so` → `out/lib/loader/{variant}/{abi}/` (sau `assembleDebug` app).

## Phụ thuộc

- Không Prefab, không phụ thuộc `:native-core` / `:app` native
- ABI: arm64-v8a, armeabi-v7a, x86, x86_64 (đồng bộ app)

## Logcat

Tag `AttackLoader` — `JNI_OnLoad`, `loaded plugin`, lỗi `dlopen` / `dlsym`.

## Lưu ý

- Plugin phải export `JNI_OnLoad` trả `JNI_VERSION_1_6`
- `RTLD_GLOBAL` để symbol plugin dùng chung với loader/process
- `currentApplication()` cần sẵn sàng khi load (cùng timing với dex init trong plugin)
- `dlopen` thử **basename** (`libattack.so`) trước, rồi `nativeLibraryDir/...` — khi `extractNativeLibs=false`, file có thể không nằm trên disk dù vẫn có trong APK
- `:app` `packaging.jniLibs.useLegacyPackaging = true` — extract .so ra `nativeLibraryDir` (AGP không cho `extractNativeLibs` trong manifest)

## Khi sửa

- Đổi tên plugin hoặc multi-plugin → chỉ `loader.cpp`
- **Không** nhét dexloader/tracker vào loader — thuộc `libattack.so`
