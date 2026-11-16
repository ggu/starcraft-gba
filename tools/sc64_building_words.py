#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Derive optional word blits from completed-building sprite frames."""

import argparse
from pathlib import Path
import re

from sc64_unit_facing import pack_word_phases


NAMES = ("photon_cannon", "pylon", "sunken_colony", "spore_colony", "hatchery",
         "lair", "hive", "nexus", "gateway", "forge", "creep_colony")
PHOTON = tuple(f"photon_cannon_frame_{i}" for i in range(4))


def emit(source: str) -> str:
    output = ["/* Private derived building frames. Do not commit. */"]
    entries = []
    packed = {}
    for name in (*NAMES, *PHOTON):
        symbol = f"sc64_race_sprite_{name}"
        frame = re.findall(
            rf"static const Sc64SpriteFrame {symbol}\s*=\s*\{{"
            r"-?\d+,-?\d+,(\d+),(\d+),\d+,\d+,(\w+)\};", source)
        if not frame and name in PHOTON:
            continue
        if len(frame) != 1:
            raise ValueError(f"missing or duplicate source frame: {symbol}")
        width, height, pixel = frame[0]
        width, height = int(width), int(height)
        bodies = re.findall(
            rf"static const unsigned char {pixel}\[(\d+)\]\s*=\s*"
            r"\{(.*?)\n\};", source, re.S)
        if len(bodies) != 1 or not 0 < width <= 256 or not 0 < height <= 256:
            raise ValueError(f"invalid source frame: {symbol}")
        count, body = bodies[0]
        tokens = body.strip().rstrip(',').split(',')
        if int(count) != len(tokens) or len(tokens) != width * height:
            raise ValueError(f"wrong pixel count: {symbol}")
        pixels = bytes(int(token, 0) for token in tokens)
        key = (width, height, pixels)
        if key not in packed:
            packed[key] = f"sc64_building_word_{len(packed)}"
            data = packed[key]
            for i, phase in enumerate(pack_word_phases(width, height, pixels)):
                output.append(f"static const u32 {data}_p{i}[] = {{")
                for k in range(0, len(phase), 8):
                    output.append("  " + ",".join(f"0x{v:08x}u" for v in phase[k:k+8]) + ",")
                output.append("};")
            ptrs = ",".join(f"{data}_p{i}" for i in range(4))
            output.append(f"static const GbaSpriteWordFrame {data} = "
                          f"{{{width},{height},{{{ptrs}}}}};")
        entries.append((symbol, packed[key], name in PHOTON))
    output += ["static const GbaSpriteWordFrame *",
               "sc64_building_word_lookup(const Sc64SpriteFrame *frame)",
               "{"]
    for src, dst, photon in entries:
        if photon:
            output.append("#ifdef SC64_PHOTON_SOURCE_FRAMES")
        output.append(f"    if (frame == &{src}) return &{dst};")
        if photon:
            output.append("#endif")
    output += ["    return 0;", "}", "#ifdef GBA_HOST",
               "static const Sc64SpriteFrame *const sc64_building_word_sources[] = {"]
    for src, _, photon in entries:
        if photon:
            output.append("#ifdef SC64_PHOTON_SOURCE_FRAMES")
        output.append(f"    &{src},")
        if photon:
            output.append("#endif")
    return "\n".join(output + ["};", "#endif"])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("sprite_bank", type=Path)
    args = parser.parse_args()
    print(emit(args.sprite_bank.read_text(encoding="ascii")))


if __name__ == "__main__":
    main()
