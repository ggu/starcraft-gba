#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Compare a saving process with the fresh process that resumed its SRAM.

Each argument is the standard output of tests/sc64_custom_larva_eggs.c in
"save" or "resume" mode. The record must have been written with Eggs in
progress on both sides and no serial queue entry, the fresh process must
restore exactly the saved world, and both must reach the same world after the
same number of updates of neutral input.
"""
from __future__ import annotations

import json
import sys


def records(path):
    out = {}
    with open(path, encoding="utf-8") as handle:
        for line in handle:
            line = line.strip()
            if not line.startswith("{"):
                continue
            row = json.loads(line)
            if row["kind"] in out:
                raise SystemExit(f"{path}: duplicate {row['kind']} record")
            out[row["kind"]] = row
    return out


def world(row):
    return {key: value for key, value in row.items() if key != "kind"}


def compare(saved, resumed):
    """Return the summary line, or raise SystemExit naming the first difference."""
    if set(saved) != {"saved", "continued"} or set(resumed) != {"loaded", "continued"}:
        raise SystemExit("expected saved+continued and loaded+continued records")
    start = saved["saved"]
    if start["human_eggs"] < 2 or start["bot_eggs"] < 2:
        raise SystemExit(f"the record holds too few Eggs: {start}")
    if start["human_queued"] or start["bot_queued"]:
        raise SystemExit(f"the record holds a serial queue entry: {start}")
    if world(start) != world(resumed["loaded"]):
        raise SystemExit(f"restored world differs from the saved one:\n{start}\n{resumed['loaded']}")
    if saved["continued"] != resumed["continued"]:
        raise SystemExit(f"continuation differs:\n{saved['continued']}\n{resumed['continued']}")
    if saved["continued"]["frame"] <= start["frame"]:
        raise SystemExit("the continuation did not advance")
    return (f"fresh-process resume: {start['human_eggs']} human and {start['bot_eggs']} computer Eggs "
            f"saved at update {start['frame']}, identical at update {saved['continued']['frame']} "
            f"(digest {saved['continued']['digest']})")


def main(argv):
    if len(argv) != 3:
        raise SystemExit(f"usage: {argv[0]} SAVED RESUMED")
    print(compare(records(argv[1]), records(argv[2])))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
