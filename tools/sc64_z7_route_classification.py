#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Validate diagnostic route-build classification against a same-source control."""
import argparse
import json
from pathlib import Path
import re

from sc64_z7_busy_attribution import parse_attribution, read_pair
from sc64_z7_busy_compare import compare_pair

CLASS_FIELDS = {"movement", "component", "depot", "empty", "evicted", "resets",
                "wraps", "distinct", "repeated", "untracked", "usable_min", "usable_max"}


def parse_classification(text, route):
    records = []
    for line in text.splitlines():
        match = re.search(r"Z7C CLASS(.*)", line)
        if not match:
            continue
        pairs = re.findall(r"(\w+)=(-?\d+)", match[1])
        if len({name for name, _ in pairs}) != len(pairs):
            raise ValueError("duplicate classification field")
        records.append({name: int(value) for name, value in pairs})
    if len(records) != 1 or set(records[0]) != CLASS_FIELDS:
        raise ValueError("missing or malformed classification record")
    row = records[0]
    if any(value < 0 for value in row.values()):
        raise ValueError("negative classification count")
    builds = route["builds"]
    if row["movement"] + row["component"] + row["depot"] != builds:
        raise ValueError("build sources do not sum to route builds")
    if row["empty"] + row["evicted"] != builds:
        raise ValueError("slot states do not sum to route builds")
    if row["distinct"] + row["repeated"] + row["untracked"] != builds:
        raise ValueError("destination counts do not sum to route builds")
    if row["distinct"] > 128 or not 0 < row["usable_min"] <= row["usable_max"] <= route["slots"]:
        raise ValueError("bad destination capacity or usable-slot range")
    return row


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--control", type=Path, required=True)
    parser.add_argument("--classification", type=Path, required=True)
    parser.add_argument("--cut", type=int, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists():
        parser.error("output already exists")
    controls, control_pins = read_pair(args.control, args.cut)
    classes, class_pins = read_pair(args.classification, args.cut)
    shared = control_pins.keys() & class_pins.keys()
    if not shared or any(control_pins[p] != class_pins[p] for p in shared):
        raise ValueError("shared control/classification inputs changed")
    summary = {}
    for variant in ("baseline", "candidate"):
        compare_pair(controls[variant], classes[variant])
        control_counts = controls[variant]["COUNTS"][0]
        class_counts = classes[variant]["COUNTS"][0]
        control_log = (args.control / ("run-" + variant) / "mgba.log").read_text(errors="replace")
        class_log = (args.classification / ("run-" + variant) / "mgba.log").read_text(errors="replace")
        control_attr = parse_attribution(control_log, args.cut, control_counts["n0"])
        class_attr = parse_attribution(class_log, args.cut, class_counts["n0"])
        a, b = control_attr["ROUTE"][0], class_attr["ROUTE"][0]
        for key in ("slots", "hits", "builds", "build_frames", "no_build_frames", "max_builds"):
            if a[key] != b[key]:
                raise ValueError("control/classification route count differs: " + key)
        control_mean = control_counts["sum0"] / control_counts["n0"]
        class_mean = class_counts["sum0"] / class_counts["n0"]
        summary[variant] = {
            "advancing_calls": class_counts["n0"],
            "control_advancing_mean_cycles": control_mean,
            "classification_advancing_mean_cycles": class_mean,
            "measured_overhead_cycles": class_mean - control_mean,
            "measured_overhead_percent": (class_mean / control_mean - 1) * 100,
            "route": b,
            "classification": parse_classification(class_log, b),
            "top": class_attr["TOP"],
        }
    if summary["baseline"]["classification"] != summary["candidate"]["classification"]:
        raise ValueError("baseline/candidate route classification differs")
    args.output.write_text(json.dumps(summary, indent=2) + "\n")
    print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    main()
