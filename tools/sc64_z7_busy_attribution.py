#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Validate and summarize optional Z7 busy timing against its control run."""
import argparse
import json
from pathlib import Path
import re

from sc64_z7_busy_compare import busy, compare_pair

STAGES = ("update", "audio", "render_present", "orders", "sight", "other_update",
          "map", "hud", "background", "buildings", "units")


def parse_attribution(text, cut, advancing):
    rows = {}
    for line in text.splitlines():
        match = re.search(r"Z7A (ROUTE|STAGE|TOP)(.*)", line)
        if not match:
            continue
        pairs = re.findall(r"(\w+)=(-?\d+)", match[2])
        if len({name for name, _ in pairs}) != len(pairs):
            raise ValueError("duplicate attribution field")
        rows.setdefault(match[1], []).append({name: int(value) for name, value in pairs})
    if {name: len(rows.get(name, [])) for name in ("ROUTE", "STAGE", "TOP")} != {
        "ROUTE": 1, "STAGE": len(STAGES), "TOP": 10
    }:
        raise ValueError("missing or duplicated attribution records")
    route = rows["ROUTE"][0]
    route_fields = {"slots", "hits", "builds", "build_frames", "no_build_frames",
                    "max_builds", "build_update", "no_build_update"}
    if set(route) != route_fields or any(value < 0 for value in route.values()):
        raise ValueError("bad route fields")
    if not 1 <= route["slots"] <= 64 or route["build_frames"] + route["no_build_frames"] != advancing:
        raise ValueError("bad route or advancing-frame counts")
    if route["max_builds"] > route["builds"] or route["build_frames"] > route["builds"]:
        raise ValueError("bad route-build counts")
    if not route["build_frames"] and route["build_update"]:
        raise ValueError("nonzero build-frame timing without builds")
    if not route["no_build_frames"] and route["no_build_update"]:
        raise ValueError("nonzero no-build timing without frames")
    for index, row in enumerate(rows["STAGE"]):
        if set(row) != {"index", "sum", "peak"} or row["index"] != index:
            raise ValueError("bad stage fields")
        if row["peak"] < 0 or row["sum"] < row["peak"]:
            raise ValueError("bad stage timing")
    previous = None
    for index, row in enumerate(rows["TOP"]):
        if set(row) != {"rank", "total", "tick", "update", "render", "builds", "hits"} or row["rank"] != index + 1:
            raise ValueError("bad top-call fields")
        if any(value < 0 for value in row.values()) or not cut < row["tick"] <= cut + 1000:
            raise ValueError("bad top-call value")
        if row["total"] < max(row["update"], row["render"]) or (previous is not None and row["total"] > previous):
            raise ValueError("bad top-call order or timing")
        previous = row["total"]
    return rows


def read_pair(directory, cut):
    before = json.loads((directory / "inputs-before.json").read_text())
    after = json.loads((directory / "inputs-after.json").read_text())
    if not before or before != after:
        raise ValueError("input manifest missing or changed")
    results = {}
    for variant in ("baseline", "candidate"):
        result = json.loads((directory / (variant + "-result.json")).read_text())
        if result.get("failures") or not result.get("observations") or not result.get("cargo_checkpoints"):
            raise ValueError("unqualified " + variant + " diagnostic")
        text = (directory / ("run-" + variant) / "mgba.log").read_text(errors="replace")
        parsed = busy(text, cut)
        if parsed != result["diagnostic"]:
            raise ValueError("recorded " + variant + " diagnostics do not match the log")
        results[variant] = parsed
    compare_pair(results["baseline"], results["candidate"])
    return results, before


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--control", type=Path, required=True)
    parser.add_argument("--attribution", type=Path, required=True)
    parser.add_argument("--cut", type=int, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if args.output.exists():
        parser.error("output already exists")
    control, control_pins = read_pair(args.control, args.cut)
    instrumented, instrumented_pins = read_pair(args.attribution, args.cut)
    shared = control_pins.keys() & instrumented_pins.keys()
    if not shared or any(control_pins[path] != instrumented_pins[path] for path in shared):
        raise ValueError("shared control/attribution inputs changed")
    summary = {}
    routes = {}
    for variant in ("baseline", "candidate"):
        compare_pair(control[variant], instrumented[variant])
        counts = instrumented[variant]["COUNTS"][0]
        text = (args.attribution / ("run-" + variant) / "mgba.log").read_text(errors="replace")
        attribution = parse_attribution(text, args.cut, counts["n0"])
        old = control[variant]["COUNTS"][0]
        control_mean = old["sum0"] / old["n0"]
        measured_mean = counts["sum0"] / counts["n0"]
        routes[variant] = attribution["ROUTE"][0]
        summary[variant] = {
            "advancing_calls": counts["n0"],
            "control_advancing_mean_cycles": control_mean,
            "attribution_advancing_mean_cycles": measured_mean,
            "measured_overhead_cycles": measured_mean - control_mean,
            "measured_overhead_percent": (measured_mean / control_mean - 1) * 100,
            "route": routes[variant],
            "stages": [{"name": name, **row} for name, row in zip(STAGES, attribution["STAGE"])],
            "top": attribution["TOP"],
        }
    for key in ("slots", "hits", "builds", "build_frames", "no_build_frames", "max_builds"):
        if routes["baseline"][key] != routes["candidate"][key]:
            raise ValueError("paired route count differs: " + key)
    args.output.write_text(json.dumps(summary, indent=2) + "\n")
    print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    main()
