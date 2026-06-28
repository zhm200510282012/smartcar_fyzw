#!/usr/bin/env python3
"""Check local C include paths without requiring a C compiler."""

from __future__ import annotations

import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
INCLUDE_RE = re.compile(r'#include\s+"([^"]+)"')
CHECK_DIRS = ["App", "BSP", "Control", "Track"]


def main() -> int:
    missing: list[tuple[str, str]] = []
    files: list[Path] = []
    for dirname in CHECK_DIRS:
        base = ROOT / dirname
        files.extend(base.rglob("*.c"))
        files.extend(base.rglob("*.h"))

    for path in files:
        text = path.read_text(encoding="utf-8", errors="ignore")
        for match in INCLUDE_RE.finditer(text):
            include = match.group(1)
            target = (path.parent / include).resolve()
            if not target.exists():
                missing.append((str(path.relative_to(ROOT)), include))

    if missing:
        print("FAIL: local include paths missing")
        for source, include in missing:
            print(f"{source} -> {include}")
        return 1

    print("PASS: local include paths resolve")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
