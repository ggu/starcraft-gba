#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Insert an ordinary pause-menu save into the P2 controller's recorded pad tape.

The input is the tape the P2 controller writes ({"events":[{"tick","held"}],"total_ticks"},
one entry per held-key transition, tick = input call index from the title screen). The output
is the same tape with the four inputs of a pause-menu save inserted before call S:

    START, neutral, B (writes SRAM), A (unpauses)

None of the four advances the world, so every later controller call plays one world update
exactly as in the uninterrupted tape. S is the first call at or after --after for which the
controller's pad is neutral on the previous and the current call, so no key is cut or
newly pressed by the insertion. Two files are written: OUT-save.json (the inserted tape) and
OUT-ref.json (the original tape plus "save_start": S, for the uninterrupted reference run).
Nothing here is derived from game data."""
import argparse
import json
import pathlib

KEY_A, KEY_B, KEY_START = 1, 2, 8
SEQUENCE = [KEY_START, 0, KEY_B, KEY_A]


def held_by_call(events, total):
    held, out, i = 0, [], 0
    for call in range(total):
        while i < len(events) and events[i]["tick"] <= call:
            held = events[i]["held"]
            i += 1
        out.append(held)
    return out


def transitions(held):
    events, prior = [], 0
    for call, value in enumerate(held):
        if value != prior or call == 0 and value:
            events.append({"tick": call, "held": value})
            prior = value
    return events


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("tape", type=pathlib.Path)
    parser.add_argument("out", type=pathlib.Path, help="output prefix")
    parser.add_argument("--after", type=int, required=True,
                        help="first call index at which the save may start")
    args = parser.parse_args()
    data = json.loads(args.tape.read_text())
    total = data["total_ticks"]
    held = held_by_call(data["events"], total)
    start = next((c for c in range(max(args.after, 1), total)
                  if held[c - 1] == 0 and held[c] == 0), None)
    if start is None:
        raise SystemExit("no neutral call after the requested point")
    inserted = held[:start] + SEQUENCE + held[start:]
    save = {"total_ticks": total + len(SEQUENCE), "save_start": start,
            "events": transitions(inserted)}
    ref = dict(data)
    ref["save_start"] = start
    args.out.parent.mkdir(parents=True, exist_ok=True)
    pathlib.Path(f"{args.out}-save.json").write_text(json.dumps(save, separators=(",", ":")) + "\n")
    pathlib.Path(f"{args.out}-ref.json").write_text(json.dumps(ref, separators=(",", ":")) + "\n")
    print(f"save inserted before call {start} (tape {total} -> {save['total_ticks']} calls)")


if __name__ == "__main__":
    main()
