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
  mod_ui_theme.cpp     Vk-Engine Moonlight theme (utils.jai) + ui scale 3×
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
- `AppUi` — `add_tab(id, label, draw)`, `set_window_title`
- Menu: **X** ImGui (`Begin(..., &p_open)`) thu gọn → FAB; bấm FAB mở lại (`set_menu_expanded`)
- Icon: chỉ `mod_ui_icon.cpp` / `mod_ui_icon.hpp` (shell/app không gọi)
- Theme: title bar căn giữa (`WindowTitleAlign` trong `mod_ui_theme.cpp`)
- Shell: sidebar tab **ribbon phải** (gradient → accent, `draw_sidebar_tab_ribbon`) + panel content (`mod_ui_shell.cpp`)

`mod_ui_internal.hpp`: `feed_*`, `begin_frame`, … — không dùng từ `:app`.

Menu size mặc định (`MenuLayoutConfig`): 624×442 dp (chữ nhật ngang), áp **một lần** khi đã có density (`try_apply_initial_menu_layout`); sau đó user resize tự do (chỉ clamp min). `TouchInputBridge.refreshInsets` gửi density.

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
