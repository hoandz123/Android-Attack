# HeroIcon — ảnh portrait nhúng sẵn (ESP ImGui)

Dữ liệu scrape từ [Tướng/Skin | Liên Quân](https://lienquan.garena.vn/hoc-vien/tuong-skin/). Mỗi tướng có byte JPEG/PNG trong `HeroIcon.cpp`.

## Luồng ESP (trong trận)

```text
ActorLinker → ConfigId
  → CHeroInfo.GetHeroName(configId)   // tên hiển thị tiếng Việt
  → HeroIcon::FindByDisplayName(name)
  → stbi_load_from_memory(iconBytes) → ImGui
```

Không hook `TryLoadSpriteAndMaterial`, không đọc texture game.

## Chạy dump lại

```powershell
cd D:\LDPlayer\1-Project\Android-Attack\app\src\main\cpp\LienQuan\Data\HeroIcon
python dump_hero_icons.py
```

## Tra cứu

```cpp
HeroIcon::FindByDisplayName("Triệu Vân");
HeroIcon::FindByFieldName("TrieuVan");
HeroIcon::FindBySlug("trieu-van");
```

`displayName` phải khớp chuỗi `CHeroInfo.GetHeroName` trong game.
