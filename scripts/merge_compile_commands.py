#!/usr/bin/env python3
"""Merge compile_commands.json from all subproject build directories."""

import json
import pathlib
import sys

root  = pathlib.Path(sys.argv[1])   # source root
build = pathlib.Path(sys.argv[2])   # cmake binary dir

candidates = [build / "compile_commands.json"] + sorted(
    root.glob("*/build/compile_commands.json")
)

merged = []
seen   = set()
for db in candidates:
    if not db.exists():
        continue
    for entry in json.loads(db.read_text()):
        f = entry["file"]
        if f not in seen:
            seen.add(f)
            merged.append(entry)

out = root / "compile_commands.json"
out.write_text(json.dumps(merged, indent=2))
print(f"gen_compile_commands: {len(merged)} entries from {len(candidates)} databases")
