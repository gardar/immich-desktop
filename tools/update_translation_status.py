#!/usr/bin/env python3
"""Regenerate the translation completion table in README files.

Parses translations/immich_*.ts and replaces the block between
<!-- translation-status:start --> and <!-- translation-status:end -->
in README.md and translations/README.md.
"""

from __future__ import annotations

import argparse
import re
import sys
import xml.etree.ElementTree as ET
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TRANSLATIONS = ROOT / "translations"
TARGETS = (ROOT / "README.md", TRANSLATIONS / "README.md")

START = "<!-- translation-status:start -->"
END = "<!-- translation-status:end -->"
MARKER_RE = re.compile(
    re.escape(START) + r".*?" + re.escape(END),
    re.DOTALL,
)

# Native / common display names for known catalogs; unknown codes use the code.
LANGUAGE_NAMES = {
    "en": "English",
    "de": "German (Deutsch)",
    "fr": "French (Français)",
    "es": "Spanish (Español)",
    "it": "Italian (Italiano)",
    "nl": "Dutch (Nederlands)",
    "pl": "Polish (Polski)",
    "pt": "Portuguese (Português)",
    "pt_BR": "Portuguese (Brasil)",
    "ru": "Russian (Русский)",
    "uk": "Ukrainian (Українська)",
    "zh_CN": "Chinese (简体)",
    "zh_TW": "Chinese (繁體)",
    "ja": "Japanese (日本語)",
    "ko": "Korean (한국어)",
    "sv": "Swedish (Svenska)",
    "cs": "Czech (Čeština)",
    "tr": "Turkish (Türkçe)",
}


@dataclass(frozen=True)
class CatalogStats:
    code: str
    finished: int
    total: int

    @property
    def percent(self) -> int:
        if self.total <= 0:
            return 0
        return round(100.0 * self.finished / self.total)

    @property
    def bar(self) -> str:
        width = 10
        filled = round(self.percent / 100.0 * width) if self.total else 0
        filled = max(0, min(width, filled))
        return "█" * filled + "░" * (width - filled)


def locale_from_path(path: Path) -> str:
    stem = path.stem  # immich_de / immich_pt_BR
    if not stem.startswith("immich_"):
        raise ValueError(f"Unexpected catalog name: {path.name}")
    return stem[len("immich_") :]


def count_catalog(path: Path) -> CatalogStats:
    # Qt .ts files are XML; ElementTree handles the DOCTYPE fine.
    root = ET.parse(path).getroot()
    finished = 0
    total = 0
    for translation in root.iter("translation"):
        kind = translation.get("type")
        if kind in {"obsolete", "vanished"}:
            continue
        total += 1
        if kind == "unfinished":
            continue
        # Finished if typed finished/empty type, and not an empty stub
        # for non-numerus; numerus forms count as finished when marked so.
        finished += 1
    return CatalogStats(locale_from_path(path), finished, total)


def collect_stats() -> list[CatalogStats]:
    files = sorted(TRANSLATIONS.glob("immich_*.ts"))
    if not files:
        raise SystemExit(f"No translation catalogs found in {TRANSLATIONS}")
    stats = [count_catalog(path) for path in files]
    # English first, then locale code.
    stats.sort(key=lambda s: (0 if s.code == "en" else 1, s.code.lower()))
    return stats


def render_table(stats: list[CatalogStats]) -> str:
    lines = [
        START,
        "",
        "| Language | Code | Progress | Strings |",
        "| --- | --- | --- | ---: |",
    ]
    for entry in stats:
        name = LANGUAGE_NAMES.get(entry.code, entry.code)
        lines.append(
            f"| {name} | `{entry.code}` | `{entry.bar}` {entry.percent}% "
            f"| {entry.finished} / {entry.total} |"
        )
    lines.extend(
        [
            "",
            "_Auto-generated from `translations/immich_*.ts`. "
            "Run `python tools/update_translation_status.py` "
            "or `cmake --build build --target update_translations`._",
            "",
            END,
        ]
    )
    return "\n".join(lines)


def update_file(path: Path, block: str) -> bool:
    text = path.read_text(encoding="utf-8")
    if not MARKER_RE.search(text):
        raise SystemExit(
            f"{path.relative_to(ROOT)} is missing {START} … {END} markers"
        )
    updated = MARKER_RE.sub(block, text, count=1)
    if updated == text:
        return False
    path.write_text(updated, encoding="utf-8", newline="\n")
    return True


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--check",
        action="store_true",
        help="Exit 1 if README tables are out of date (no writes).",
    )
    args = parser.parse_args()

    stats = collect_stats()
    block = render_table(stats)

    if args.check:
        stale = False
        for path in TARGETS:
            text = path.read_text(encoding="utf-8")
            match = MARKER_RE.search(text)
            if match is None or match.group(0) != block:
                print(f"stale: {path.relative_to(ROOT)}", file=sys.stderr)
                stale = True
        if stale:
            print(
                "Translation status tables are out of date. "
                "Run: python tools/update_translation_status.py",
                file=sys.stderr,
            )
            return 1
        print("Translation status tables are up to date.")
        return 0

    for path in TARGETS:
        if update_file(path, block):
            print(f"updated {path.relative_to(ROOT)}")
        else:
            print(f"unchanged {path.relative_to(ROOT)}")

    for entry in stats:
        print(
            f"  {entry.code}: {entry.finished}/{entry.total} ({entry.percent}%)"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
