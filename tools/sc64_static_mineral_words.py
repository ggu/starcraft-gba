#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Prepare four alignment phases for the three shared static mineral frames."""

import argparse
from pathlib import Path
import re

from sc64_unit_facing import pack_word_phases


SOURCES = tuple(f"sc64_sprite_mineral_field_{i}" for i in (1, 2, 3))


def frame_data(source: str, symbol: str) -> tuple[int, int, bytes]:
    frame = re.findall(
        rf"static const Sc64SpriteFrame {symbol}\s*=\s*\{{"
        r"\s*-?\d+\s*,\s*-?\d+\s*,\s*(\d+)\s*,\s*(\d+)\s*,"
        r"\s*\d+\s*,\s*\d+\s*,\s*(\w+)\s*\};", source)
    if len(frame) != 1:
        raise ValueError(f"missing or duplicate source frame: {symbol}")
    width, height, pixel = frame[0]
    width, height = int(width), int(height)
    if not 0 < width <= 256 or not 0 < height <= 256:
        raise ValueError(f"invalid source frame dimensions: {symbol}")
    arrays = re.findall(
        rf"static const (?:u8|unsigned char) {pixel}\[(\d+)\]\s*=\s*"
        r"\{(.*?)\};", source, re.S)
    if len(arrays) != 1:
        raise ValueError(f"missing or duplicate source pixels: {symbol}")
    size, body = arrays[0]
    pixels = bytes(int(token, 0) for token in body.strip().rstrip(",").split(","))
    if int(size) != len(pixels) or len(pixels) != width * height:
        raise ValueError(f"wrong source pixel count: {symbol}")
    return width, height, pixels


def emit(source: str) -> str:
    out = ["/* Private derived mineral frames. Do not commit. */"]
    entries = []
    for i, symbol in enumerate(SOURCES, 1):
        width, height, pixels = frame_data(source, symbol)
        name = f"sc64_static_mineral_words_{i}"
        for phase, words in enumerate(pack_word_phases(width, height, pixels)):
            out.append(f"static const u32 {name}_p{phase}[] = {{")
            for k in range(0, len(words), 8):
                out.append("    " + ",".join(f"0x{v:08x}u" for v in words[k:k + 8]) + ",")
            out.append("};")
        ptrs = ",".join(f"{name}_p{p}" for p in range(4))
        out.append(f"static const GbaSpriteWordFrame {name} = "
                   f"{{{width},{height},{{{ptrs}}}}};")
        entries.append((symbol, name))
    out += ["static const GbaSpriteWordFrame *",
            "sc64_static_mineral_word_lookup(const Sc64SpriteFrame *frame)", "{"]
    for symbol, name in entries:
        out.append(f"    if (frame == &{symbol}) return &{name};")
    return "\n".join(out + ["    return 0;", "}", ""])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("sprite_bank", type=Path)
    args = parser.parse_args()
    print(emit(args.sprite_bank.read_text(encoding="ascii")), end="")


if __name__ == "__main__":
    main()
