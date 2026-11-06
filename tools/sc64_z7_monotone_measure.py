#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Compare the opt-in exact route probe with a same-source Z7 busy control."""
import argparse
import json
from pathlib import Path
import re

from sc64_z7_busy_attribution import parse_attribution, read_pair
from sc64_z7_busy_compare import compare_pair
from sc64_z7_route_classification import parse_classification


def parse_startup(text, counts):
    rows = {}
    for line in text.splitlines():
        match = re.search(r"Z7B (STARTUP|STACK)(.*)", line)
        if not match:
            continue
        pairs = re.findall(r"(\w+)=(-?\d+)", match[2])
        if len({name for name, _ in pairs}) != len(pairs):
            raise ValueError("duplicate startup field")
        rows.setdefault(match[1], []).append({name: int(value) for name, value in pairs})
    if {k: len(rows.get(k, [])) for k in ("STARTUP", "STACK")} != {"STARTUP": 1, "STACK": 1}:
        raise ValueError("missing or duplicated startup/stack record")
    startup, stack = rows["STARTUP"][0], rows["STACK"][0]
    if set(startup) != {"load", "first100_sum", "first100_peak", "later900_sum"}:
        raise ValueError("bad startup fields")
    if any(value <= 0 for value in startup.values()) or startup["first100_peak"] > startup["first100_sum"]:
        raise ValueError("bad startup timing")
    if startup["first100_sum"] + startup["later900_sum"] != sum(counts[f"sum{i}"] for i in range(3)):
        raise ValueError("startup timing does not sum to calls")
    if set(stack) != {"painted", "untouched"} or not 0 < stack["untouched"] <= stack["painted"]:
        raise ValueError("invalid stack watermark")
    return {"startup": startup, "stack": stack}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--control", type=Path, required=True)
    parser.add_argument("--shortcut", type=Path, required=True)
    parser.add_argument("--cut", type=int, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists():
        parser.error("output already exists")
    control, control_pins = read_pair(args.control, args.cut)
    shortcut, shortcut_pins = read_pair(args.shortcut, args.cut)
    shared = control_pins.keys() & shortcut_pins.keys()
    if not shared or any(control_pins[p] != shortcut_pins[p] for p in shared):
        raise ValueError("shared control/shortcut inputs changed")
    summary = {}
    for variant in ("baseline", "candidate"):
        compare_pair(control[variant], shortcut[variant])
        sides = {}
        for name, directory, records in (("control", args.control, control),
                                          ("shortcut", args.shortcut, shortcut)):
            counts = records[variant]["COUNTS"][0]
            text = (directory / ("run-" + variant) / "mgba.log").read_text(errors="replace")
            attr = parse_attribution(text, args.cut, counts["n0"])
            route = attr["ROUTE"][0]
            sides[name] = {"counts": counts, "route": route,
                           "classification": parse_classification(text, route),
                           **parse_startup(text, counts), "top": attr["TOP"]}
        old = sides["control"]["counts"]
        new = sides["shortcut"]["counts"]
        old_mean, new_mean = old["sum0"] / old["n0"], new["sum0"] / new["n0"]
        summary[variant] = {"control": sides["control"], "shortcut": sides["shortcut"],
                            "advancing_mean_difference": new_mean - old_mean,
                            "advancing_mean_percent": (new_mean / old_mean - 1) * 100,
                            "route_build_difference": sides["shortcut"]["route"]["builds"] - sides["control"]["route"]["builds"]}
    for side in ("control", "shortcut"):
        a, b = (summary[v][side] for v in ("baseline", "candidate"))
        if a["classification"] != b["classification"]:
            raise ValueError("paired route classification differs: " + side)
    args.output.write_text(json.dumps(summary, indent=2) + "\n")
    print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    main()
