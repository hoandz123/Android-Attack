#!/usr/bin/env python3
"""
Scrape hero portraits from lienquan.garena.vn, download images, embed as byte arrays.

Run from this directory:
    python dump_hero_icons.py

Requires: Python 3.8+ (stdlib only).
"""

from __future__ import annotations

import html as html_module
import re
import sys
import unicodedata
import urllib.error
import urllib.request
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path

LIST_URL = "https://lienquan.garena.vn/hoc-vien/tuong-skin/"
USER_AGENT = (
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
    "AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36"
)
BYTES_PER_LINE = 14

CARD_RE = re.compile(
    r'<a\s[^>]*href="([^"]*?/tuong-skin/d/([^"/]+)/)"[^>]*class="st-heroes__item"[^>]*>'
    r'[\s\S]*?<img[^>]+src="([^"]+)"[^>]*alt="([^"]*)"[^>]*>'
    r'[\s\S]*?<h2[^>]*>\s*([^<]*?)\s*</h2>',
    re.IGNORECASE,
)

HERE = Path(__file__).resolve().parent
OUT_H = HERE / "HeroIcon.h"
OUT_CPP = HERE / "HeroIcon.cpp"


@dataclass(frozen=True)
class HeroRow:
    field_name: str
    display_name: str
    slug: str
    icon_url: str
    image_bytes: bytes


def strip_accents(text: str) -> str:
    text = text.replace("\u0110", "D").replace("\u0111", "d")
    nfd = unicodedata.normalize("NFD", text)
    return "".join(c for c in nfd if unicodedata.category(c) != "Mn")


APOSTROPHES = (
    "'", '"', "`",
    "\u2018", "\u2019", "\u201a", "\u201b",  # ‘ ’ ‚ ‛
    "\u2032", "\u2035",  # ′ ‵
)


def display_to_field(display_name: str, slug: str, used: set[str]) -> str:
    raw = strip_accents(display_name)
    for ch in APOSTROPHES:
        raw = raw.replace(ch, "")
    parts = re.split(r"[\s\-–—]+", raw.strip())
    parts = [p for p in parts if p]
    base = "".join(p[:1].upper() + p[1:].lower() for p in parts) if parts else "Hero"
    base = re.sub(r"[^0-9A-Za-z]", "", base)
    if not base or base[0].isdigit():
        base = "Hero" + base
    field = f"{base}2" if slug.endswith("-2") else base
    n = 2
    while field in used:
        field = f"{base}{n}"
        n += 1
    used.add(field)
    return field


def escape_cpp(s: str) -> str:
    return s.replace("\\", "\\\\").replace('"', '\\"')


def fetch_html(url: str) -> str:
    req = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
    with urllib.request.urlopen(req, timeout=45) as resp:
        return resp.read().decode("utf-8", errors="replace")


def fetch_bytes(url: str) -> bytes:
    req = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
    with urllib.request.urlopen(req, timeout=60) as resp:
        return resp.read()


def parse_heroes(html: str) -> list[tuple[str, str, str, str]]:
    matches = list(CARD_RE.finditer(html))
    if not matches:
        raise RuntimeError(
            "Khong tim thay the tuong trong HTML. "
            "Cap nhat CARD_RE trong dump_hero_icons.py."
        )
    used_fields: set[str] = set()
    out: list[tuple[str, str, str, str]] = []
    for m in matches:
        slug = m.group(2).strip()
        icon_url = m.group(3).strip()
        display_name = html_module.unescape(m.group(5).strip())
        if not display_name or not icon_url:
            continue
        field = display_to_field(display_name, slug, used_fields)
        out.append((field, display_name, slug, icon_url))
    return out


def format_byte_array(var: str, data: bytes) -> list[str]:
    lines = [f"static const uint8_t {var}[] = {{"]
    for i in range(0, len(data), BYTES_PER_LINE):
        chunk = data[i : i + BYTES_PER_LINE]
        hexes = ", ".join(f"0x{b:02x}" for b in chunk)
        lines.append(f"    {hexes},")
    lines.append("};")
    return lines


def generate_header(count: int) -> str:
    return f"""#pragma once

#include <cstddef>
#include <cstdint>

namespace lienquan {{
namespace HeroIcon {{

/** Embedded portrait (JPEG/PNG). Decode: stbi_load_from_memory(iconBytes, iconSize, ...). */
struct Entry {{
    const char *fieldName;
    const char *displayName;
    const char *slug;
    const uint8_t *iconBytes;
    size_t iconSize;
}};

inline constexpr size_t kHeroCount = {count};

extern const Entry kAll[kHeroCount];

const Entry *FindByDisplayName(const char *displayName) noexcept;
const Entry *FindBySlug(const char *slug) noexcept;
const Entry *FindByFieldName(const char *fieldName) noexcept;

}} // namespace HeroIcon
}} // namespace lienquan
"""


def generate_cpp(rows: list[HeroRow], generated_at: str) -> str:
    blob_sections: list[str] = []
    entry_lines: list[str] = []

    for r in rows:
        var = f"k{r.field_name}IconBytes"
        blob_sections.extend(format_byte_array(var, r.image_bytes))
        blob_sections.append("")
        entry_lines.append(
            f'    {{"{escape_cpp(r.field_name)}", "{escape_cpp(r.display_name)}", '
            f'"{escape_cpp(r.slug)}", {var}, sizeof({var})}},'
        )

    blobs = "\n".join(blob_sections)
    entries = "\n".join(entry_lines)

    return f"""#include "HeroIcon.h"
#include <cstring>

// Generated by dump_hero_icons.py at {generated_at}
// Source: {LIST_URL}

namespace lienquan {{
namespace HeroIcon {{

namespace {{

{blobs}

}} // namespace

const Entry kAll[kHeroCount] = {{
{entries}
}};

const Entry *FindByDisplayName(const char *displayName) noexcept {{
    if (!displayName || !displayName[0]) return nullptr;
    for (size_t i = 0; i < kHeroCount; ++i)
        if (std::strcmp(kAll[i].displayName, displayName) == 0) return &kAll[i];
    return nullptr;
}}

const Entry *FindBySlug(const char *slug) noexcept {{
    if (!slug || !slug[0]) return nullptr;
    for (size_t i = 0; i < kHeroCount; ++i)
        if (std::strcmp(kAll[i].slug, slug) == 0) return &kAll[i];
    return nullptr;
}}

const Entry *FindByFieldName(const char *fieldName) noexcept {{
    if (!fieldName || !fieldName[0]) return nullptr;
    for (size_t i = 0; i < kHeroCount; ++i)
        if (std::strcmp(kAll[i].fieldName, fieldName) == 0) return &kAll[i];
    return nullptr;
}}

}} // namespace HeroIcon
}} // namespace lienquan
"""


def main() -> int:
    print(f"Fetching {LIST_URL} ...")
    try:
        html = fetch_html(LIST_URL)
    except urllib.error.URLError as e:
        print(f"ERROR: {e}", file=sys.stderr)
        return 1

    parsed = parse_heroes(html)
    print(f"Parsed {len(parsed)} heroes. Downloading images ...")

    rows: list[HeroRow] = []
    for i, (field, display, slug, url) in enumerate(parsed, 1):
        try:
            data = fetch_bytes(url)
        except urllib.error.URLError as e:
            print(f"ERROR [{field}] {url}: {e}", file=sys.stderr)
            return 1
        if len(data) < 16:
            print(f"ERROR [{field}] image too small ({len(data)} bytes)", file=sys.stderr)
            return 1
        rows.append(
            HeroRow(
                field_name=field,
                display_name=display,
                slug=slug,
                icon_url=url,
                image_bytes=data,
            )
        )
        print(f"  [{i}/{len(parsed)}] {field}: {len(data)} bytes")

    total = sum(len(r.image_bytes) for r in rows)
    print(f"Total embedded: {total / 1024 / 1024:.2f} MiB")

    generated_at = datetime.now(timezone.utc).strftime("%Y-%m-%d %H:%M:%S UTC")
    OUT_H.write_text(generate_header(len(rows)), encoding="utf-8", newline="\n")
    OUT_CPP.write_text(generate_cpp(rows, generated_at), encoding="utf-8", newline="\n")

    print(f"Wrote {OUT_H}")
    print(f"Wrote {OUT_CPP}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
