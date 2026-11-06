#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Compare every HOST Z7 replay call's selected-state/display fingerprints."""
import argparse
from pathlib import Path
import struct

import sc64_z7_arm_replay as replay

RECORD = struct.Struct("=4I3Q")
LABELS = ("call", "game frame", "screen", "mode", "actor hash", "world hash", "display hash")


def compare(control: Path, shortcut: Path, expected_calls: int) -> int:
    """Fail on a missing, duplicated or differing call; no game bytes are exported."""
    if expected_calls <= 0:
        raise ValueError("expected call count must be positive")
    with control.open("rb") as first, shortcut.open("rb") as second:
        for call in range(1, expected_calls + 1):
            a, b = first.read(RECORD.size), second.read(RECORD.size)
            if len(a) != RECORD.size or len(b) != RECORD.size:
                raise ValueError(f"truncated frame trace at call {call}")
            av, bv = RECORD.unpack(a), RECORD.unpack(b)
            if av[0] != call or bv[0] != call:
                raise ValueError(f"missing or reordered frame trace at call {call}")
            for label, left, right in zip(LABELS, av, bv):
                if left != right:
                    raise ValueError(f"{label} differs at call {call}: {left} vs {right}")
        if first.read(1) or second.read(1):
            raise ValueError("frame trace has unexpected trailing data")
    return expected_calls


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--trace", type=Path, required=True, help="authenticated Z7 input tape")
    parser.add_argument("--control", type=Path, required=True)
    parser.add_argument("--shortcut", type=Path, required=True)
    args = parser.parse_args()
    plan = replay.load_trace(args.trace)
    count = compare(args.control, args.shortcut, plan["stop"])
    print(f"matched {count} per-call selected-state/display fingerprints")


if __name__ == "__main__":
    main()
