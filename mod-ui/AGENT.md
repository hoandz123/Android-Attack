# mod-ui — ImGui shell + render (Prefab `modui`)

## Vai trò

Library native: khung menu ImGui, render GLES, API C++ cho plugin đăng ký menu. **Không** chứa logic game/app cụ thể.

## Prefab

- Publish: `modui` (headers `src/main/cpp/include/mod_ui.hpp`)
- Consume: `:app` link `mod-ui::modui` + static lib từ prefab package

## Cấu trúc

```
src/main/cpp/
  include/mod_ui.hpp           API public (plugin :app)
  include/mod_ui_internal.hpp  JNI bridge + input/render (chỉ trong :mod-ui)
  mod_ui.cpp           init / glue
  mod_ui_theme.cpp     colors + metrics + ui scale
  mod_ui_layout.cpp    menu size/pos theo dp (density từ Java)
  include/mod_ui_layout.hpp  MenuLayoutConfig
  mod_ui_shell.cpp     layout menu
  mod_ui_render.cpp    GLES + ImGui backend
  surface_bridge.cpp   JNI com.android.attack.Surface → set_surface / frames
  fonts/FreeSans.ttf   nguồn TTF
  fonts/mod_ui_font_data.h  TTF nhúng (python scripts/embed_font.py)
  CMakeLists.txt
```

Phụ thuộc compile: `-DNATIVE_CORE_CPP` (imgui headers từ native-core).

## API public (`mod_ui.hpp`)

- `init()` — ImGui context + JNI `RegisterNatives` (gọi `register_surface_natives` trong `surface_bridge.cpp`)
- `AppUi` / `set_app_ui()` / `app_ui()` — plugin menu (`app_menu.cpp`)

`mod_ui_internal.hpp`: `feed_*`, `begin_frame`, … — không dùng từ `:app`.

Menu size mặc định (`MenuLayoutConfig`): 320×400 dp, clamp theo `WorkSize` và min/max dp. `TouchInputBridge.refreshInsets` gửi `DisplayMetrics.density`.

## Build

```bash
./gradlew :mod-ui:assembleDebug
```

Cần `:native-core` (imgui) đã build cho Prefab imgui khi link app.

## Phụ thuộc

- `native-core` imgui, EGL/GLES (link ở app)
- Không phụ thuộc `:native-dex`, `:loader`

## Khi sửa

- Giao diện menu chung → `mod_ui_shell.cpp` / `mod_ui_render.cpp`
- API header → `include/mod_ui.hpp`
- Widget riêng từng app → module `:app` (`app_menu.cpp`), không sửa mod-ui nếu không chia sẻ

## Logcat

Theo tag trong implementation (thường cùng nhóm ImGui init từ app plugin).

## Lưu ý

- Static lib `libmodui.a` merge vào `libattack.so`, không load riêng
- Giữ tách biệt: mod-ui = framework UI; app = nội dung menu
