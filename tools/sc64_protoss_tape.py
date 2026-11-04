#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Convert a Protoss campaign controller trace to the form tools/sc64_p8_arm_replay.py reads.

P1 and P2 write {"events":[{"tick","held"}],"total_ticks"} with the call index in
"tick". P3 to P6 write {"events":[...]} whose input rows give "host_tick" after the
call. P7 writes the target form with its own final record; P8 to P10 pass through.
"""
import argparse
import json
import pathlib

KEY_MASK = 1023


def rows_from_race(data: dict) -> tuple[list[tuple[int, int]], int]:
    return [(row["tick"], row["held"]) for row in data["events"]], data["total_ticks"]


def rows_from_events(data: dict) -> tuple[list[tuple[int, int]], int]:
    inputs, stop = [], None
    for row in data["events"]:
        if row.get("kind") == "input":
            inputs.append((row["host_tick"] - 1, row["held"]))
        elif "host_tick" in row:
            stop = row["host_tick"]
    if stop is None:
        raise ValueError("trace has no final record")
    return inputs, stop


def convert(source: pathlib.Path, out: pathlib.Path, mission: int) -> None:
    text = source.read_text()
    if text.lstrip().startswith('{"kind":"metadata"'):
        lines = text.splitlines()
        final = next((json.loads(row) for row in lines if '"kind":"final"' in row), None)
        if final is not None:
            lines.append(json.dumps({"kind": "checkpoint", "label": "final",
                                     "call_index": final["total_calls"], "won": final["won"],
                                     "lost": final["lost"], "frame": final["frame"]}))
        out.write_text("\n".join(lines) + "\n")
        return
    data = json.loads(text)
    inputs, stop = rows_from_race(data) if "total_ticks" in data else rows_from_events(data)
    lines = [json.dumps({"kind": "metadata", "schema": 1, "mission": mission,
                         "profile": source.name})]
    held_before = 0
    for tick, held in inputs:
        if held & ~KEY_MASK:
            raise ValueError(f"tick {tick}: key mask {held}")
        if held == held_before or tick >= stop:
            continue
        lines.append(json.dumps({"kind": "input", "call_index": tick, "held": held,
                                 "pressed": held & ~held_before}))
        held_before = held
    lines.append(json.dumps({"kind": "checkpoint", "label": "final", "call_index": stop,
                             "won": 1, "lost": 0, "frame": 0}))
    out.write_text("\n".join(lines) + "\n")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=pathlib.Path)
    parser.add_argument("out", type=pathlib.Path)
    parser.add_argument("--mission", type=int, required=True)
    args = parser.parse_args()
    convert(args.source, args.out, args.mission)


if __name__ == "__main__":
    main()
