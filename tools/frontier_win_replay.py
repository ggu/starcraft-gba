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
CARTRIDGES = ("foundry", "rovers", "bulwark")
CPU_DEFAULTS = (1, 2, 0)
TITLE_UPDATE = 12
SETUP_UPDATE = 100
KEY_A, KEY_RIGHT, KEY_LEFT, KEY_DOWN = 1, 16, 32, 128


def case_inputs(case, cartridge):
    edges = [tuple(edge) for edge in case["button_edges"]]
    if cartridge == "foundry":
        return edges, SETUP_UPDATE, case["end"]

    launch = next((i for i, (_, mask) in enumerate(edges) if mask & KEY_A), None)
    if launch is None or edges[launch][1] != KEY_A or edges[launch][0] >= SETUP_UPDATE:
        raise ValueError("case must launch from the title with A before the setup checkpoint")

    taps = [KEY_RIGHT] if case["map"] else []
    taps.append(KEY_DOWN)  # Opponent count starts at two.
    bots = len(case["cpu"])
    if bots != 2:
        taps.append(KEY_LEFT if bots == 1 else KEY_RIGHT)
    taps.append(KEY_DOWN)  # Human faction starts at the cartridge's faction.
    taps.extend([KEY_RIGHT] * ((case["player"] - CARTRIDGES.index(cartridge)) % len(CARTRIDGES)))
    for default, wanted in zip(CPU_DEFAULTS, case["cpu"]):
        taps.append(KEY_DOWN)
        taps.extend([KEY_RIGHT] * ((wanted - default) % len(CARTRIDGES)))
    if case.get("cpu_team", False):
        taps.extend([KEY_DOWN, KEY_RIGHT])

    title_edges = []
    for i, tap in enumerate(taps):
        tick = TITLE_UPDATE + 2 * i
        title_edges.extend(((tick, tap), (tick + 1, 0)))
    shift = TITLE_UPDATE + 2 * len(taps) - edges[launch][0]
    field_edges = [(tick + shift, mask) for tick, mask in edges[launch:]]
    return title_edges + field_edges, SETUP_UPDATE + shift, case["end"] + shift


def read_value(replay, sym, sizes, name, size):
    if name not in sym or sizes.get(name) != size:
        raise ValueError(f"missing or unexpected symbol {name}")
    return int.from_bytes(replay.read(sym[name], size), "little", signed=False)


def run_case(case, runner, output, sym, sizes, cartridge="foundry"):
    cpu = case["cpu"]
    if not isinstance(cpu, list) or not 1 <= len(cpu) <= 3:
        raise ValueError("cpu must contain one to three faction IDs")
    cpu_team = case.get("cpu_team", False)
    if type(cpu_team) is not bool:
        raise ValueError("cpu_team must be a boolean")
    edges, setup_update, end = case_inputs(case, cartridge)
    if end <= setup_update:
        raise ValueError("case must continue beyond the setup checkpoint")

    rom = ROOT / f"build/Frontier-{cartridge.title()}-GBA.gba"
    proc, replay, log = physical.start(runner, rom, output, sym, sizes)
    try:
        replay.play_to(TITLE_UPDATE, edges)
        if (read_value(replay, sym, sizes, "screen", 4) != 0 or
                read_value(replay, sym, sizes, "frontier_human_faction", 1) != CARTRIDGES.index(cartridge)):
            raise RuntimeError(f"{case['name']}: wrong title state for {cartridge}")
        replay.play_to(setup_update, edges)
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
        if "frontier_cpu_team" in sym or cpu_team:
            if read_value(replay, sym, sizes, "frontier_cpu_team", 4) != int(cpu_team):
                raise RuntimeError(f"{case['name']}: wrong CPU team mode")
        if "cpu_team" in case:
            setup["cpu_team"] = cpu_team
        start_cycle = replay.clock()
        replay.play_to(end, edges)
        cycles = replay.clock() - start_cycle
        if cycles <= 0:
            raise RuntimeError(f"{case['name']}: emulator clock did not advance")
        result = {name: read_value(replay, sym, sizes, name, 4)
                  for name in ("screen", "won", "lost", "frame")}
        expected_result = {"screen": 3, "won": 1, "lost": 0,
                           "frame": case["game_tick"]}
        if result != expected_result:
            raise RuntimeError(f"{case['name']}: result {result} != {expected_result}")
        updates = end - setup_update
        timing = {"after_update": setup_update, "updates": updates,
                  "emulated_cycles": cycles,
                  "updates_per_second": round(GBA_CLOCK_HZ * updates / cycles, 2)}
        return {"name": case["name"], "setup": setup, "end": end,
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
    parser.add_argument("--cartridge", type=str.lower, choices=CARTRIDGES,
                        default="foundry", help="Frontier cartridge to replay (default: foundry)")
    args = parser.parse_args()
    cases = json.loads((ROOT / "tests/frontier_win_inputs.json").read_text())
    if not isinstance(cases, list) or not cases:
        raise ValueError("input must be a nonempty list of cases")
    if args.case is not None:
        cases = [case for case in cases if case.get("name") == args.case]
        if len(cases) != 1:
            raise ValueError("--case must match exactly one case")
    elf = ROOT / f"build/frontier-{args.cartridge}.elf"
    sym, sizes = symbols(elf), symbol_sizes(elf)
    args.output.mkdir(parents=True, exist_ok=False)
    results = []
    for index, case in enumerate(cases):
        case_dir = args.output / f"case-{index + 1}"
        case_dir.mkdir()
        results.append(run_case(case, args.runner, case_dir, sym, sizes, args.cartridge))
    (args.output / "result.json").write_text(json.dumps(results, indent=2) + "\n")


if __name__ == "__main__":
    main()
