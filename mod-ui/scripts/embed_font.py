#!/usr/bin/env python3
"""Regenerate mod_ui_font_data.h from fonts/FreeSans.ttf."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "src/main/cpp/fonts/FreeSans.ttf"
DST = ROOT / "src/main/cpp/fonts/mod_ui_font_data.h"

def main() -> None:
    data = SRC.read_bytes()
    lines = [
        "#pragma once",
        "#include <cstddef>",
        "#include <cstdint>",
        "",
        f"inline constexpr std::size_t kModUiFontSize = {len(data)};",
        "inline const unsigned char kModUiFontData[] = {",
    ]
    for i in range(0, len(data), 16):
        chunk = data[i : i + 16]
        lines.append("    " + ", ".join(f"0x{b:02x}" for b in chunk) + ",")
    lines.append("};")
    lines.append("")
    DST.write_text("\n".join(lines), encoding="utf-8")
    print(f"wrote {DST} ({len(data)} bytes)")


if __name__ == "__main__":
    main()
