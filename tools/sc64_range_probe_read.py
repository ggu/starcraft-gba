#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Read an authored range-probe ROM using physical zero-button input.

Requires the existing read-only mGBA backend and an instrumented ELF/ROM pair.
No debugger memory writes, game-world injection or SRAM import/export is used.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import statistics
import subprocess

from emulator_smoke import symbol_sizes, symbols
from sc64_headless_physical import PhysicalReplay


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("runner", "rom", "elf", "output"):
        parser.add_argument("--" + name, type=Path, required=True)
    parser.add_argument("--ticks", type=int, default=100)
    args = parser.parse_args()
    if not 1 <= args.ticks <= 10000:
        parser.error("--ticks must be between 1 and 10000")
    args.output.mkdir(parents=True, exist_ok=False)
    syms, sizes = symbols(args.elf), symbol_sizes(args.elf)
    names = ("demo_frame", "demo_last_cycles", "demo_update_cycles",
             "probe_tick", "probe_fault", "probe_first_move", "probe_first_hit")
    for name in names:
        if sizes.get(name) != 4:
            raise ValueError(f"missing four-byte probe symbol: {name}")
    proc, replay, log = PhysicalReplay.start(
        args.runner, args.rom, args.output, syms, sizes)
    samples = []
    try:
        for tick in range(1, args.ticks + 1):
            replay.play_to(tick, [])
            row = {name: int.from_bytes(replay.read(syms[name], 4), "little")
                   for name in names}
            if (row["demo_frame"] != tick or row["probe_tick"] != tick or
                    row["probe_fault"]):
                raise RuntimeError(f"invalid probe at update {tick}: {row}")
            samples.append(row)
    finally:
        proc.terminate()
        try:
            proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            proc.kill()
            proc.wait()
        log.close()
    cycles = [row["demo_last_cycles"] for row in samples]
    result = {
        "scope": "input + update + audio + render, including present wait; "
                 "authored scene, not hardware or full-match FPS",
        "hashes": {name: hashlib.sha256(path.read_bytes()).hexdigest()
                   for name, path in (("runner", args.runner), ("rom", args.rom),
                                      ("elf", args.elf))},
        "samples": samples,
        "cycles_mean": statistics.mean(cycles),
        "cycles_peak": max(cycles),
        "cycles_p95": sorted(cycles)[(95 * len(cycles) + 99) // 100 - 1],
        "above_two_refresh_frames": sum(c > 2 * 280896 for c in cycles),
    }
    (args.output / "result.json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps({key: value for key, value in result.items()
                      if key != "samples"}, sort_keys=True))


if __name__ == "__main__":
    main()
