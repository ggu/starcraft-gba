#!/usr/bin/env python3
"""Exercise the hidden showcase menu and eight scenes with controller input.

Requires the optional showcase build and mGBA. Captures and SRAM stay local.
Only demo_inject_keys is written; this is not physical GBA qualification.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import struct

import sc64_menu_smoke as menu
import sc64_split_profile as profile
import sc64_split_smoke as split
from emulator_smoke import capture, symbols

A, B, SELECT, START, DOWN, R, L = 1, 2, 4, 8, 128, 256, 512


def run(elf: Path, rom: Path, out: Path) -> None:
    out.mkdir(parents=True, exist_ok=False)
    sym = symbols(elf)
    fields = split.frontend_offsets(elf)
    proc, sock, rsp, log = split.start_emulator("showcase", rom, out, sym)
    results = []
    try:
        original_write = rsp.write

        def input_only(address, data):
            if address != sym["demo_inject_keys"] or len(data) != 2:
                raise RuntimeError("only controller input may be written")
            return original_write(address, data)

        rsp.write = input_only

        def number(name):
            return struct.unpack("<i", rsp.read(sym[name], 4))[0]

        def front(name):
            return menu.u32(rsp, sym["race_front"] + fields[name])

        def held(keys):
            rsp.write(sym["demo_inject_keys"], struct.pack("<H", keys))
            rsp.tick(2)

        def press(keys):
            held(keys)
            held(0)

        held(R)
        held(R | L)
        held(R | L | SELECT)
        held(0)
        assert number("screen") == 0 and front("page") == 5
        rsp.tick(80)
        capture(rsp, out / "menu.png")
        for scene in range(8):
            assert front("row") == scene
            press(A)
            assert number("screen") == 2 and number("sc64_showcase_scene") == scene
            assert number("sc64_showcase_ticks") == 0
            if scene in (0, 1):
                capture(rsp, out / f"scene-{scene}-ready.png")
            press(A)
            assert number("screen") == 1
            start_frame = number("frame")
            clock = profile.hardware_clock(rsp)
            rsp.tick(120)
            end_frame = number("frame")
            elapsed = (profile.hardware_clock(rsp) - clock) & 0xFFFFFFFF
            result = {"scene": scene, "sample_updates": end_frame - start_frame,
                      "effective_fps": round(profile.CPU_HZ * (end_frame-start_frame) / elapsed, 3),
                      "sample_frame": end_frame, "lost": number("lost")}
            capture(rsp, out / f"scene-{scene}-action.png")
            if scene in (2, 3, 4):
                while number("frame") < 149:
                    rsp.tick(1)
                assert not number("lost")
                rsp.tick(1)
                assert number("lost") and number("screen") == 3
                assert number("sc64_showcase_ticks") == 150
                result["loss_update"] = 150
                capture(rsp, out / f"scene-{scene}-defeat.png")
            # Restart from a pause/result using the same controller path.
            if number("screen") == 1:
                press(START)
                press(R)
            else:
                press(A)
            assert number("screen") == 2 and number("sc64_showcase_ticks") == 0
            press(SELECT)
            assert number("screen") == 0 and front("page") == 5 and front("row") == scene
            results.append(result)
            if scene < 7:
                press(DOWN)
        (out / "result.json").write_text(json.dumps({
            "rom_sha256": hashlib.sha256(rom.read_bytes()).hexdigest(),
            "input_only": True, "scenes": results,
            "limits": "Staged setups; carrier approach has no Interceptor attacks; emulator timing only",
        }, indent=2) + "\n")
        print(json.dumps(results, indent=2))
    finally:
        menu.stop_emulator(proc, sock, log)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--elf", type=Path, default=Path("build/starcraft-showcase.elf"))
    parser.add_argument("--rom", type=Path, default=Path("build/StarCraft-Showcase-Transport1-GBA.gba"))
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()
    run(args.elf, args.rom, args.out)
