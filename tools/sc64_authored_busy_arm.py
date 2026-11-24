#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Build and compare an asset-free GBA crowd on the normal frame loop."""

import argparse
from hashlib import sha256
import json
from pathlib import Path
import statistics
import subprocess
import sys

from emulator_smoke import symbols, symbol_sizes
from sc64_headless_physical import PhysicalReplay


ROOT = Path(__file__).resolve().parents[1]
FIXTURE = ROOT / "tests/sc64_authored_busy_arm.c"
TWO_VBLANKS = 561_792
SOURCES = (
    "common/start.s",
    "common/main.c",
    "common/gba.c",
    "common/memory.c",
    "common/audio_bank.S",
    "starcraft/sc64_bc_body_art.c",
    "starcraft/sc64_interceptor_motion.c",
    "starcraft/sc64_interceptor_pool_codec.c",
)
STATE = ("u", "b", "m", "tracers", "visible", "explored", "skirmish_ai", "seed",
         "cx", "cy")
PAN_INPUT = [(20, 16), (39, 0), (76, 32), (95, 0)]


def build(tree, output, arm_scene_clip):
    elf = output / "crowd.elf"
    rom = output / "crowd.gba"
    cpu = ["-mcpu=arm7tdmi", "-mthumb", "-mthumb-interwork"]
    link = ["-nostdlib", "-Tcommon/gba.ld", "-Wl,--gc-sections"]
    flags = [
        "-O2", "-g", "-std=c11", "-ffreestanding", "-fno-builtin",
        "-fno-strict-aliasing", "-fno-common", "-ffunction-sections",
        "-fdata-sections", "-Icommon", "-Istarcraft",
        "-DGBA_PRESENT_VBLANK_EPOCH", "-DSC64_SKIRMISH",
        "-DSC_ORIGINAL_SKIRMISH", "-DSC_FRONTIER_ROUTE_CACHE",
    ]
    if arm_scene_clip:
        flags += ["-DGBA_SCENE_CLIP", "-DGBA_SHAPES_ROM_ARM"]
    with (output / "build.log").open("w") as log:
        subprocess.run(
            ["arm-none-eabi-gcc", *cpu, *flags, *SOURCES, str(FIXTURE),
             *link, "-lgcc", "-o", str(elf)],
            cwd=tree, stdout=log, stderr=subprocess.STDOUT, check=True,
        )
        subprocess.run(
            ["arm-none-eabi-objcopy", "-O", "binary", elf, rom], check=True,
        )
        subprocess.run(
            [sys.executable, ROOT / "tools/fix_rom.py", rom, "BUSY TEST", "XB01"],
            stdout=log, stderr=subprocess.STDOUT, check=True,
        )
    return rom, elf


def summary(samples):
    ordered = sorted(samples)
    percentile = (len(ordered) * 95 + 99) // 100 - 1
    return {
        "mean": round(statistics.mean(ordered)),
        "p95": ordered[percentile],
        "max": ordered[-1],
        "over_two_vblanks": sum(value > TWO_VBLANKS for value in ordered),
    }


def replay(tree, runner, output, updates, arm_scene_clip, inputs):
    output.mkdir(parents=True, exist_ok=False)
    rom, elf = build(tree, output, arm_scene_clip)
    address, sizes = symbols(elf), symbol_sizes(elf)
    process, remote, log = PhysicalReplay.start(runner, rom, output, address, sizes)

    def word(name):
        return int.from_bytes(remote.read(address[name], 4), "little")

    def state():
        digest = sha256()
        for name in STATE:
            digest.update(remote.read(address[name], sizes[name]))
        return digest.hexdigest()

    try:
        samples = []
        for tick in range(1, updates + 1):
            remote.play_to(tick, inputs)
            if word("busy_setup_fault") or word("busy_initial_count") != 76:
                raise RuntimeError("Crowd setup failed")
            if word("screen") != 1 or word("frame") != tick:
                raise RuntimeError(f"Play frame did not advance at tick {tick}")
            samples.append({
                "frame": word("demo_last_cycles"),
                "update": word("demo_update_cycles"),
                "draw": word("gfx_draw_cycles"),
                "state": state(),
                "camera": [word("camx"), word("camy")],
                "pages": [sha256(remote.read(page, 38_400)).hexdigest()
                          for page in (0x06000000, 0x0600A000)],
                "palette": sha256(remote.read(0x05000000, 512)).hexdigest(),
                "display": int.from_bytes(remote.read(0x04000000, 2), "little"),
            })
    finally:
        process.terminate()
        process.wait()
        log.close()

    result = {
        "checkout": subprocess.check_output(
            ["git", "-C", str(tree), "rev-parse", "HEAD"], text=True
        ).strip(),
        "tracked_diff_sha256": sha256(subprocess.check_output(
            ["git", "-C", str(tree), "diff", "--binary", "HEAD"]
        )).hexdigest(),
        "rom_sha256": sha256(rom.read_bytes()).hexdigest(),
        "summary": {stage: summary([row[stage] for row in samples])
                    for stage in ("frame", "update", "draw")},
        "samples": samples,
    }
    (output / "result.json").write_text(json.dumps(result, indent=2) + "\n")
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--baseline", required=True, type=Path)
    parser.add_argument("--candidate", required=True, type=Path)
    parser.add_argument("--runner", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--updates", type=int, default=120)
    parser.add_argument("--arm-scene-clip", action="store_true",
                        help="Use the ARM shape wrappers and the scene clip")
    parser.add_argument("--pan", action="store_true",
                        help="Move the viewport right and back during the run")
    args = parser.parse_args()
    output = args.output.resolve()
    runner = args.runner.resolve()
    if args.updates < 1:
        parser.error("--updates must be positive")
    if not runner.is_file():
        parser.error(f"Replay runner not found: {runner}")
    inputs = PAN_INPUT if args.pan else []
    baseline = replay(args.baseline.resolve(), runner, output / "baseline", args.updates,
                      args.arm_scene_clip, inputs)
    candidate = replay(args.candidate.resolve(), runner, output / "candidate", args.updates,
                       args.arm_scene_clip, inputs)
    fields = ("state", "camera", "pages", "palette", "display")
    differences = {field: [] for field in fields}
    for tick, (old, new) in enumerate(zip(baseline["samples"], candidate["samples"]), 1):
        for field in fields:
            if old[field] != new[field]:
                differences[field].append(tick)
    result = {
        "runner_sha256": sha256(runner.read_bytes()).hexdigest(),
        "arm_scene_clip": args.arm_scene_clip,
        "input_changes": inputs,
        "baseline": baseline["summary"],
        "candidate": candidate["summary"],
        "differences": {field: ticks for field, ticks in differences.items() if ticks},
    }
    (output / "comparison.json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps(result, indent=2))
    return bool(result["differences"])


if __name__ == "__main__":
    raise SystemExit(main())
