#!/usr/bin/env python3
"""Verify that files listed in the Keil uvproj exist."""

from __future__ import annotations

import sys
import xml.etree.ElementTree as ET
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
UVPROJ = ROOT / "MDK_Project" / "smartcar_fyzw.uvproj"


def main() -> int:
    tree = ET.parse(UVPROJ)
    missing: list[str] = []
    for node in tree.findall(".//FilePath"):
        if not node.text:
            continue
        rel = node.text.replace("\\", "/")
        path = (UVPROJ.parent / rel).resolve()
        if not path.exists():
            missing.append(node.text)
    if missing:
        print("FAIL: uvproj file paths missing")
        for item in missing:
            print(item)
        return 1
    print("PASS: uvproj file paths exist")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
