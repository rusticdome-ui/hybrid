#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Host regression for the upstream GHOULBOX Volca phrase insertion."""
import importlib.util
from pathlib import Path
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location("ac79_link_engines", ROOT / "tools/ac79_link_engines.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)

ANCHOR = '''    {"TOMBBEAT", {36, 42, 0, 42, 45, 42, 36, 42, 36, 42, 45, 45, 38, 42, 39, 42},       /* 22 a procession */
     {1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0}},
'''
NAMES = ("VOLCBASS", "VOLCKEYS", "VOLCDRUM", "VOLCFM", "VOLCSAMP")

def main():
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        source = root / "firmware/src/engines.c"
        source.parent.mkdir(parents=True)
        source.write_text("static const struct PATTERNS[] = {\n" + ANCHOR + "};\n")
        MODULE.apply_volca_patterns(root)
        first = source.read_text()
        for name in NAMES:
            assert first.count('{"' + name + '"') == 1, name
        for name in NAMES:
            start = first.index('{"' + name + '"')
            end = first.index("}},", start) if "}}, " in first[start:] else first.index("}},", start)
            record = first[start:end + 3]
            assert record.count(",") >= 32, (name, record)
        MODULE.apply_volca_patterns(root)
        assert source.read_text() == first, "insertion must be idempotent"
        assert first.index('{"TOMBBEAT"') < first.index('{"VOLCBASS"')
    print("SMK-37 VOLCA factory phrase insertion: PASS (5 patterns, stable IDs, idempotent)")

if __name__ == "__main__":
    main()
