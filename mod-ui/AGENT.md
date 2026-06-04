# mod-ui — ImGui shell + render (Prefab `modui`)

## Vai trò

Library native: khung menu ImGui, render GLES, API C++ cho plugin đăng ký menu. **Không** chứa logic game/app cụ thể.

## Prefab

- Publish: `modui` (headers `src/main/cpp/include/ModUi.hpp`)
- Consume: `:app` link `mod-ui::modui` + static lib từ prefab package

## Cấu trúc

```
src/main/cpp/
  include/ModUi.hpp           API public (plugin :app)
  include/Internal.hpp        JNI bridge + input/render (chỉ trong :mod-ui)
  ModUi.cpp                   Init / glue
  Theme.cpp                   Vk-Engine Moonlight theme (utils.jai) + ui scale 3×
  Layout.cpp                  menu size/pos theo dp (density từ Java)
  include/Layout.hpp          MenuLayoutConfig
  Shell.cpp                   layout menu
  Render.cpp                  GLES + ImGui backend
  SurfaceBridge.cpp           JNI com.android.attack.Surface → SetSurface / frames
  fonts/FreeSans.ttf          nguồn TTF
  fonts/FontData.h            TTF nhúng (python scripts/embed_font.py)
  CMakeLists.txt
```

Phụ thuộc compile: `-DNATIVE_CORE_CPP` (imgui headers từ native-core).

## API public (`ModUi.hpp`)

- `Init()` — ImGui context + JNI `RegisterNatives` (gọi `RegisterSurfaceNatives` trong `SurfaceBridge.cpp`)
- `AppUi` — `set_window_title`, `add_tab(id, label, draw)` (≤16 tab); field `menu_size` (dp, {0,0}=mặc định), `icon_url` (URL tải FAB; cache local `/data/user/0/<pkg>/files/fab.png` trong `Icon.cpp`)
- `SetAppUi(ui)` / `GetAppUi()` — **đăng ký** menu (app dựng `AppUi` rồi gọi `SetAppUi`); `RegisterOverlayDraw(fn)` — vẽ overlay thêm
- `SetMenuVisible/MenuVisible`, `SetMenuExpanded/MenuExpanded` — bật/tắt + thu gọn FAB
- Menu: **X** ImGui (`Begin(..., &p_open)`) thu gọn → FAB; bấm FAB mở lại (`SetMenuExpanded`)
- Icon: chỉ `Icon.cpp` / `Icon.hpp` (shell/app không gọi)
- Theme: title bar căn giữa (`WindowTitleAlign` trong `Theme.cpp`)
- Shell: sidebar tab **ribbon phải** (gradient → accent, `DrawSidebarTabRibbon`) + panel content (`Shell.cpp`)

`Internal.hpp`: `Feed*`, `BeginFrame`, … — không dùng từ `:app`.

Menu size mặc định (`MenuLayoutConfig`): 624×442 dp (chữ nhật ngang), áp **một lần** khi đã có density (`ApplyInitialLayout`); sau đó user resize tự do (chỉ clamp min). `TouchInputBridge.refreshInsets` gửi density.

## Build

```bash
./gradlew :mod-ui:assembleDebug
```

Cần `:native-core` (imgui) đã build cho Prefab imgui khi link app.

## Phụ thuộc

- `native-core` imgui, EGL/GLES (link ở app)
- Không phụ thuộc `:native-dex`, `:loader`

## Khi sửa

- Giao diện menu chung → `Shell.cpp` / `Render.cpp`
- API header → `include/ModUi.hpp`
- Widget riêng từng app → module `:app` (`Menu.cpp`), không sửa mod-ui nếu không chia sẻ

## Logcat

Theo tag trong implementation (thường cùng nhóm ImGui init từ app plugin).

## Lưu ý

- Static lib `libmodui.a` merge vào `libattack.so`, không load riêng
- Giữ tách biệt: mod-ui = framework UI; app = nội dung menu
