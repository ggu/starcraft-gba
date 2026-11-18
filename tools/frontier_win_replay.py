#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Replay Frontier win cases with physical buttons and read-only witnesses."""
import argparse
import json
from pathlib import Path
import subprocess

from emulator_smoke import symbol_sizes, symbols
import sc64_headless_physical as physical

ROOT = Path(__file__).resolve().parents[1]


def read_value(replay, sym, sizes, name, size):
    if name not in sym or sizes.get(name) != size:
        raise ValueError(f"missing or unexpected symbol {name}")
    return int.from_bytes(replay.read(sym[name], size), "little", signed=False)


def run_case(case, runner, output, sym, sizes):
    proc, replay, log = physical.start(
        runner, ROOT / "build/Frontier-Foundry-GBA.gba", output, sym, sizes
    )
    try:
        edges = [tuple(edge) for edge in case["button_edges"]]
        if case["end"] < 100:
            raise ValueError("case ends before setup checkpoint")
        replay.play_to(100, edges)
        cpu = case["cpu"]
        if not isinstance(cpu, list) or not 1 <= len(cpu) <= 3:
            raise ValueError("cpu must contain one to three faction IDs")
        setup = {
            "screen": read_value(replay, sym, sizes, "screen", 4),
            "map": read_value(replay, sym, sizes, "original_skirmish_map", 4),
            "player": read_value(replay, sym, sizes, "frontier_human_faction", 1),
            "bots": read_value(replay, sym, sizes, "skirmish_bots", 4),
        }
        if sizes.get("frontier_cpu_faction") != 3:
            raise ValueError("missing or unexpected symbol frontier_cpu_faction")
        setup["cpu"] = list(replay.read(sym["frontier_cpu_faction"], len(cpu)))
        expected = {"screen": 1, "map": case["map"], "player": case["player"],
                    "bots": len(cpu), "cpu": cpu}
        if setup != expected:
            raise RuntimeError(f"{case['name']}: setup {setup} != {expected}")
        replay.play_to(case["end"], edges)
        result = {name: read_value(replay, sym, sizes, name, 4)
                  for name in ("screen", "won", "lost", "frame")}
        expected_result = {"screen": 3, "won": 1, "lost": 0,
                           "frame": case["game_tick"]}
        if result != expected_result:
            raise RuntimeError(f"{case['name']}: result {result} != {expected_result}")
        return {"name": case["name"], "setup": setup, "end": case["end"],
                "result": result}
    finally:
        proc.terminate()
        try:
            proc.wait(timeout=3)
        except subprocess.TimeoutExpired:
            proc.kill()
            proc.wait()
        log.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("runner", type=Path, help="mGBA physical observer executable")
    parser.add_argument("--output", type=Path, required=True,
                        help="new directory for observer logs and results")
    parser.add_argument("--case", help="run only the case with this name")
    args = parser.parse_args()
    cases = json.loads((ROOT / "tests/frontier_win_inputs.json").read_text())
    if not isinstance(cases, list) or not cases:
        raise ValueError("input must be a nonempty list of cases")
    if args.case is not None:
        cases = [case for case in cases if case.get("name") == args.case]
        if len(cases) != 1:
            raise ValueError("--case must match exactly one case")
    elf = ROOT / "build/frontier-foundry.elf"
    sym, sizes = symbols(elf), symbol_sizes(elf)
    args.output.mkdir(parents=True, exist_ok=False)
    results = []
    for index, case in enumerate(cases):
        case_dir = args.output / f"case-{index + 1}"
        case_dir.mkdir()
        results.append(run_case(case, args.runner, case_dir, sym, sizes))
    (args.output / "result.json").write_text(json.dumps(results, indent=2) + "\n")


if __name__ == "__main__":
    main()
