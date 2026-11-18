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
GBA_CLOCK_HZ = 16_777_216
SETUP_UPDATE = 100


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
        if case["end"] <= SETUP_UPDATE:
            raise ValueError("case must continue beyond the setup checkpoint")
        replay.play_to(SETUP_UPDATE, edges)
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
        cpu_team = case.get("cpu_team", False)
        if type(cpu_team) is not bool:
            raise ValueError("cpu_team must be a boolean")
        if "frontier_cpu_team" in sym or cpu_team:
            if read_value(replay, sym, sizes, "frontier_cpu_team", 4) != int(cpu_team):
                raise RuntimeError(f"{case['name']}: wrong CPU team mode")
        if "cpu_team" in case:
            setup["cpu_team"] = cpu_team
        start_cycle = replay.clock()
        replay.play_to(case["end"], edges)
        cycles = replay.clock() - start_cycle
        if cycles <= 0:
            raise RuntimeError(f"{case['name']}: emulator clock did not advance")
        result = {name: read_value(replay, sym, sizes, name, 4)
                  for name in ("screen", "won", "lost", "frame")}
        expected_result = {"screen": 3, "won": 1, "lost": 0,
                           "frame": case["game_tick"]}
        if result != expected_result:
            raise RuntimeError(f"{case['name']}: result {result} != {expected_result}")
        updates = case["end"] - SETUP_UPDATE
        timing = {"after_update": SETUP_UPDATE, "updates": updates,
                  "emulated_cycles": cycles,
                  "updates_per_second": round(GBA_CLOCK_HZ * updates / cycles, 2)}
        return {"name": case["name"], "setup": setup, "end": case["end"],
                "result": result, "timing": timing}
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
