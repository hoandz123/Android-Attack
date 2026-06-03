# LienQuan — game mod (Liên Quân Mobile)

## Tổng quan

| | |
|---|---|
| Game | **Liên Quân Mobile** (Garena / Arena of Valor) |
| Package | `com.garena.game.kgvn` |
| Engine | Unity **IL2CPP** |
| Đăng ký | `REGISTER_GAME("com.garena.game.kgvn", Activate)` trong `LienQuan.cpp` |
| Trạng thái | **STUB / khung** — chỉ menu demo, **chưa có hook thật** |

## Cấu trúc

```
LienQuan/
  LienQuan.cpp     toàn bộ logic (entry + menu demo + InitHooks rỗng)
```

Chưa có `Config/`, `SDK/`, `UI/` riêng. Khi mở rộng nên theo cấu trúc của `PlayTogether/` (xem `PlayTogether/AGENT.md`).

## Entry & luồng

`lienquan::Activate()` (gọi bởi `games::Dispatch` khi package khớp):

1. `InitHooks()` — **hiện rỗng** (chỉ có comment mẫu `HOOK_LIB(...)`).
2. Menu `modui::AppUi` 480×340 — 1 tab **Chính** (`DrawMainTab`) với 2 checkbox demo (Aim assist, Map hack) **chưa nối logic**.

> Khác PlayTogether: LienQuan cài hook **đồng bộ** ngay trong `Activate()`, không tách thread `Init_Il2cpp_Symbol`. Nếu cần IL2CPP symbol, thêm `Init_Il2cpp_Symbol()` + check `il2cpp_loaded` như PlayTogether.

## Stub có sẵn dùng được

`PlayTogether/Stubs/ESPManager.h` (header-only) — quản lý `ESPEntry` (vị trí world/screen, tên, màu) + `TickStale` để vẽ ESP. Hữu ích nếu làm ESP cho LienQuan; render qua `native-core::gameui` (`GameUI/EspGUI`).

## Khi mở rộng

1. Hook thật → điền `InitHooks()`: `HOOK_LIB("libil2cpp.so", "0xRVA", cb, old_cb)` (offset) hoặc `Tools::Hook(GET_METHOD(...), ...)` (theo tên class/method nếu có dump).
2. Tìm RVA/signature → skill `il2cpp-call-flow-analysis`; dump đặt tại `dump/com.garena.game.kgvn/` (xem [dump/README.md](../../../../../../dump/README.md)).
3. Có nhiều tính năng → tách `Config/`, `SDK/`, `UI/` theo mẫu PlayTogether.
4. Build: `./gradlew :app:assembleDebug`.

## Logcat

Tag hiện dùng chung `ATTACK_PlayTogether` (nên đổi thành `ATTACK_LienQuan` khi phát triển thật).
