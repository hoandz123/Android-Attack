# Dump IL2CPP theo package

Mỗi game một thư mục con = `package` Android (tránh lẫn `dump.cs` / `libil2cpp.so` giữa các game).

## Cấu trúc

```
dump/
  README.md
  com.vng.playtogether/
    com.vng.playtogether_<version>_[arm64-v8a].cs   # Il2CppDumper
    libil2cpp.so                                    # binary cùng bản APK (local, thường không commit)
  com.haegin.playtogether/                          # bản global — cùng game, package khác
  com.garena.game.kgvn/                             # Liên Quân (stub) — khi có dump
```

## Quy ước đặt file

| File | Đặt tại |
|------|---------|
| `dump.cs` từ Il2CppDumper | `dump/<package>/` — giữ tên có version + ABI, ví dụ `com.vng.playtogether_2.27.1_[arm64-v8a].cs` |
| `libil2cpp.so` (phân tích xref/disasm) | Cùng thư mục package, cùng version với file `.cs` |

Phân tích IL2CPP (skill / script): trỏ `--dump` và `--so` vào đúng thư mục `dump/<package>/`.

## Play Together (VNG)

- Package: `com.vng.playtogether`
- Loader Zygisk cũng nhận `com.haegin.playtogether` — dump global để trong `dump/com.haegin.playtogether/` khi có.

## Liên Quân

- Package: `com.garena.game.kgvn`
- Thư mục `dump/com.garena.game.kgvn/` dành cho dump tương lai; không dùng chung với Play Together.
