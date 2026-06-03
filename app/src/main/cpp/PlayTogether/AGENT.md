# PlayTogether — game mod (Play Together)

## Tổng quan

| | |
|---|---|
| Game | **Play Together** (VNG) |
| Package | `com.vng.playtogether` |
| Engine | Unity **IL2CPP** (arm64) |
| Đăng ký | `REGISTER_GAME("com.vng.playtogether", Activate)` trong `PlayTogether.cpp` |
| Trạng thái | **Đầy đủ** — auto câu cá + chế mồi + lọc/bán cá + fake vùng |

Tính năng trọng tâm: **tự động câu cá** vòng kín (cast → bắt → lọc → bán/giữ → chế mồi khi hết).

## Entry & luồng

`PlayTogether.cpp` → `playtogether::Activate()` (gọi bởi `games::Dispatch` khi package khớp):

1. Dựng menu `modui::AppUi` — 1 tab **Câu Cá** (`DrawCauCaPage`), FAB icon `/data/user/0/<pkg>/files/fab.png`.
2. Thread nền: `Init_Il2cpp_Symbol()` → nếu `il2cpp_loaded` → `Hook::init()`.

`Hook::init()` (`Hook/Hook.cpp`) — guard `s_initOnce`, rồi: `LoadConfig` → `AutoFishing::InitPickerCache` → `AntiCheat::init` → thread nền null `FrameWork.AntiCheatListener` → cài tất cả hook qua `Tools::Hook(GET_METHOD(...), cb, &old_*)`.

**Nhịp tick chính** = hook `ActorControl::get_Kunit` (game gọi liên tục). Mỗi tick lọc `get_IsMyActor`, chờ loading xong (`isGameLoading`), rồi gọi `AutoFishing::Update()` (~400ms/lần, `RATE_LIMIT`) + cập nhật picker.

## Cấu trúc thư mục

```
PlayTogether/
  PlayTogether.cpp          entry + REGISTER_GAME + menu
  Config/Config.{h,cpp}     PLConfig (JSON), gPLConfig, isGameLoading
  UI/Tab/CauCa.{h,cpp}      ImGui tab "Câu Cá" (Câu / Lọc / Bán Cá / Nâng cao)
  Hook/
    Hook.{h,cpp}            cài hook + tick driver
    AntiCheat.{h,cpp}       NOP/RET patch chống phát hiện
    AutoFishing/            FSM câu cá + PickerSnapshot (combo mồi/vùng/recipe)
    SDK/                    wrapper IL2CPP class của game (xem bảng dưới)
  Stubs/ESPManager.h        quản lý entry ESP (CHƯA dùng trong PlayTogether)
```

## Tính năng ↔ Config ↔ Code

Config: `PLConfig::fishing` (`Config.h`), lưu JSON tại `/storage/emulated/0/Android/data/<pkg>/config.json`. UI sửa field → `SaveConfig()` ngay.

| Tính năng | Config field | Code |
|-----------|--------------|------|
| Auto câu (FSM) | `enabled`, `stopWhenCountOver` | `AutoFishing::Update` |
| Đóng dialog thưởng | `autoCloseReward` | `handleRewardDialog` |
| Lọc cá theo bóng (1–7) | `filterByShadow`, `keepShadow[7]` | `keepCurrentFish` / `trySkipFish` |
| Lọc cá theo level | `filterByLevel`, `keepLevels` (CSV) | `keepCurrentFish` |
| Bán cá theo bóng | `sellByShadow`, `sellShadow[7]` | `handleRewardDialog` |
| Bán cá theo độ hiếm (grade 1–5) | `sellByGrade`, `sellGrade[5]` | `handleRewardDialog` |
| Tự gắn mồi | `autoEquipBait`, `baitItemId` | `tryAutoEquipBait` |
| Tự chế mồi | `autoCraftBait`, `craftBaitItemId`, `craftBaitTargetCount` | `tryAutoCraftBait` → `CombineContent` |
| Fake vùng câu | `fakeZoneEnabled`, `fakeZoneId` | `onSendToFishingCasting` / `onShowFishingZoneTitle` |

FSM câu theo `eFishingState`: `None`→`StartFishing`; `Idle/Search/Hit/Fighting`→ `FishingHit`/`Lift`/skip (`FishLeave`/`FishingMiss`); `Boast/Finish`→ xử lý dialog; `Fail/Miss/CastingFail`→ retry sau delay. Lọc đọc level/bóng từ `TableFishingDifficultyImpl`.

## Hook đã cài (`Hook.cpp`)

| Hook | Việc |
|------|------|
| `ActorControl::get_Kunit` | tick driver: cache my_Player/Unit/Motor + gate loading + gọi Update |
| `FishingSystem::ReceiveFishingCatch` | lưu `CatchItemID` để quyết định bán |
| `NetNativeProtocol::PID_FISHING_CASTING` | lưu `FishingDifficultyID` (fallback level) |
| `NetNativeProtocol::SendToFishingCasting` | patch list zone → fake vùng trước khi gửi |
| `ActorDefaultControlPlayer::ShowFishingZoneTitle` | đồng bộ fake zone khi hiện title |
| `NetNativeProtocol::PID_Crafting_Start/TimeDecrease/ItemReward` | FSM chế mồi slot |
| `NetNativeProtocol::PID_ITEM_Combine` | FSM chế mồi combine tức thì |
| `CombineContent::ShowCraftingRewardDialog` | **noop** → reward âm thầm |
| `NetWebProtocol::PID_SHOP_Buy/PID_SHOP_BuyList` | ack mua nguyên liệu |
| `DialogBaseLock::installHooks()` | bỏ khóa UI khi gửi protocol |

## AntiCheat (`AntiCheat.cpp`)

`MemoryPatch::createWithHex(method->methodPointer, HEX_*)`:
- `HEX_NOP`: `InsectSystem.CheckInsectTeleport`, `AntiCheatListener.Awake/LateUpdate`, `ModuleSystem.CheckCheatDetect`.
- `HEX_RET_FALSE`: `AntiCheatListener.get_IsCheating/CheckDetect/OnSpeedHack*/OnTimeCheatingDetected/OnTableCheatingDetected`.
- Runtime: thread trong `Hook::init` set `FrameWork.AntiCheatListener = null`.

## SDK IL2CPP (`Hook/SDK/`)

Mỗi file bọc 1 class game: `FindClass("X")` + `invoke_method`/`get_field_*`. Sửa khi cần method/field mới.

| Nhóm | Class wrapper |
|------|---------------|
| Core/actor | `FrameWork`, `SystemHelper`, `ActorControl`, `CacheSystem`, `CacheUser`, `ContentSystem` |
| Fishing | `FishingSystem`, `TableFishingDifficultyImpl`, `TableFishingAreaImpl`, `TableFishingBaitImpl` |
| Bảng item | `TableSystem`, `TableItemImpl`, `TableMessagesImpl`, `TableRecipeImpl`, `TableItemCraftingListImpl`, `TableIngredientGroupImpl`, `TableIngredientImpl` |
| Crafting | `CombineContent` (FSM), `UserItemCraftingSlot`, `ItemCraftingStartA`, `ItemCraftingTimeDecreaseA`, `ItemCraftingRewardItemA` |
| Network | `NetNativeProtocol`, `NetWebProtocol`, `ProtocolA`, `NetworkRewardPack` |
| Dialog | `DialogBaseLock` |
| `enum/` | `eFishingState`, `Item_Type`, `Fish_Type`, `eFishingFailType`, … (POD enum, không wrapper) |

### CombineContent — FSM chế mồi (`SDK/CombineContent.cpp`)

`TryInstantCombine(recipeId, cookCount, itemId)`:
1. Thu slot đã xong trước (kể cả craft tay).
2. `CraftingTime==0` → **combine tức thì** (`FindIngredientItems` → `SendToItemCombine`); `>0` → **slot có thời gian** (`SendToCraftingCreate` → `TimeDecrease` finish ngay → reward).
3. Thiếu nguyên liệu → `tryBuy` (`PID_SHOP_BuyList`) rồi retry sau ack.

State trong `g_p` (Pending) / `g_buy` (BuyPending); poll qua `isCraftInFlight()`. AutoFishing gọi khi `have < target`.

## PickerSnapshot (`AutoFishing/PickerSnapshot.{h,cpp}`)

Snapshot thread-safe (game thread ghi, UI thread đọc qua `ReadPicker`) cho combo: danh sách **mồi** trong túi, **vùng câu**, **công thức mồi**. UI gọi `NotifyPickerOpen/Closed`, `NotifyCraftPanelVisible`; game thread `UpdatePickerFromGameThread` (trong `ActorControl::get_Kunit`).

## Quy ước quan trọng

- **Mọi action game gọi trên game thread** (trong tick `get_Kunit`), không gọi từ UI/render thread.
- `isGameLoading` gate — bỏ qua khi đang load (tránh crash/đụng state chưa sẵn sàng).
- Chống spam: `RATE_LIMIT(ms)`, `canActNow()`, cooldown per-action trong `g_time`.
- Chuỗi bọc `OBF(...)`; con trỏ hook `RetType (*old_Foo)(...)`; dùng `Object`/`List`/`Dictionary` từ `API/Il2cpp_Struct.h` (xem `.cursor/rules/code-style.mdc`).

## Khi sửa

- Thêm tính năng câu → `AutoFishing.cpp` (+ field `Config.h` + UI `CauCa.cpp`).
- Thêm hook → `Hook.cpp` (`GET_METHOD` + `old_*` trong SDK file tương ứng).
- Class/method game mới → thêm wrapper trong `SDK/` (dùng skill `il2cpp-call-flow-analysis` để tìm signature/RVA).
- Build: `./gradlew :app:assembleDebug` (theo `.cursor/rules/build-check.mdc`).

## Logcat

`ATTACK_PlayTogether`, `ATTACK_AutoFishing`, `ATTACK_CombineContent`.
