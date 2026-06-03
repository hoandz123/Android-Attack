# LienQuan — game mod (Liên Quân Mobile)

**Nhánh phát triển:** `LienQuan` (từ `main`). Deploy mặc định: `attack.pushTarget=com.garena.game.kgvn` trong `gradle.properties`.

## Tổng quan

| | |
|---|---|
| Game | **Liên Quân Mobile** (Garena / Arena of Valor) |
| Package | `com.garena.game.kgvn` |
| Engine | Unity **IL2CPP** |
| Đăng ký | `REGISTER_GAME("com.garena.game.kgvn", Activate)` trong `LienQuan.cpp` |
| Trạng thái | **STUB / khung** — menu demo; hook IL2CPP chờ dump/RVA |
| Logcat | `ATTACK_LienQuan` |

## Cấu trúc

```
LienQuan/
  LienQuan.cpp     toàn bộ logic (entry + menu demo + InitHooks rỗng)
```

Chưa có `Config/`, `SDK/`, `UI/` riêng. Khi mở rộng nên theo cấu trúc của `PlayTogether/` (xem `PlayTogether/AGENT.md`).

## Entry & luồng

`lienquan::Activate()` (gọi bởi `games::Dispatch` khi package khớp):

1. Menu `modui::AppUi` 480×340 — tab **Chính** (`DrawMainTab`), checkbox demo chưa nối logic.
2. Thread nền: `Init_Il2cpp_Symbol()` → nếu `il2cpp_loaded` → `InitHooks()` (**hiện rỗng**, mẫu `HOOK_LIB(...)`).

## Stub có sẵn dùng được

`PlayTogether/Stubs/ESPManager.h` (header-only) — quản lý `ESPEntry` (vị trí world/screen, tên, màu) + `TickStale` để vẽ ESP. Hữu ích nếu làm ESP cho LienQuan; render qua `native-core::gameui` (`GameUI/EspGUI`).

## Khi mở rộng

1. Hook thật → điền `InitHooks()`: `HOOK_LIB("libil2cpp.so", "0xRVA", cb, old_cb)` (offset) hoặc `Tools::Hook(GET_METHOD(...), ...)` (theo tên class/method nếu có dump).
2. Tìm RVA/signature → skill `il2cpp-call-flow-analysis`; dump đặt tại `dump/com.garena.game.kgvn/` (xem [dump/README.md](../../../../../../dump/README.md)).
3. Có nhiều tính năng → tách `Config/`, `SDK/`, `UI/` theo mẫu PlayTogether.
4. Build: `./gradlew :app:assembleDebug`.

## Logcat

`adb logcat -s ATTACK_LienQuan`
