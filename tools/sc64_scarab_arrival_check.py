#!/usr/bin/env python3
"""Check retained OpenBW arrival traces; does not extract or distribute assets."""
# SPDX-License-Identifier: GPL-3.0-or-later
import argparse
import hashlib
import json
import re
from pathlib import Path


def fields(line):
    return dict(re.findall(r"(\w+)=(-?\d+)", line))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("manifest", type=Path)
    args = parser.parse_args()
    rows = json.loads(args.manifest.read_text())
    expected = {(s, str(p)) for s in ("east", "south", "far", "diagonal", "north", "west", "slope", "turn") for p in range(8)}
    seen = set()
    for row in rows:
        scenario, phase = row["argv"][-2:]
        key = scenario, phase
        if key not in expected or key in seen or row["exit"]:
            raise ValueError(f"invalid case {key}")
        seen.add(key)
        arrival, damage = row["arrival"], row["damage"]
        payload = (args.manifest.parent / f"{scenario}-{phase}-v2.log").read_bytes()
        if hashlib.sha256(payload).hexdigest() != row["sha256"]:
            raise ValueError(f"trace hash mismatch {key}")
        lines = payload.decode().splitlines()
        children, bullets = {}, {}
        for line in lines:
            if line.startswith("observe frame="):
                f = fields(line); children[int(f["frame"])] = f
            elif line.startswith("bullet frame="):
                f = fields(line)
                if int(f["weapon"]) == 82: bullets[int(f["frame"])] = f
        if damage != arrival + 5 or not all(f in children for f in range(arrival, arrival + 12)):
            raise ValueError(f"missing child timeline {key}")
        if int(children[arrival]["state"]) != 6 or not (int(children[arrival]["child_signal"]) & 1):
            raise ValueError(f"arrival signal mismatch {key}")
        if int(children[arrival + 1]["shot_order"]) != 0 or int(children[arrival + 11]["child_sprite"]) != 0:
            raise ValueError(f"death timing mismatch {key}")
        for offset in range(5):
            b = bullets.get(arrival + offset)
            if not b or int(b["state"]) != 5 or int(b["wait"]) != 4 - offset:
                raise ValueError(f"weapon Death animation mismatch {key}, tick {offset}")
    if seen != expected:
        raise ValueError(f"missing cases: {sorted(expected - seen)}")
    print(f"OpenBW arrival: {len(seen)} source traces match signal, Die, bullet and damage timeline")


if __name__ == "__main__":
    main()
