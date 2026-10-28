#!/usr/bin/env python3
# Copyright (C) 2026 blizzard-gba contributors
# SPDX-License-Identifier: GPL-3.0-or-later
"""Build private campaign HUD icons from the verified StarCraft 64 ROM.

The source ROM, icon pixels, generated header and report remain below
.local/sc64/interface/generated. The pinned sc64-maps decoder supplies BOLT
archive access; this file implements only the well-documented GRP row codec.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
ROM_SHA1 = "a36d91b5aac7bb95df0d7de5f9814f5270e25c2b"
REFERENCE_COMMIT = "09b57f787a247107d1de82b8c5eab2734667ea1d"
ICON_GRP = "011/016"
WORLD_PALETTE = "00B/004"
DEFAULT_REMAP = ROOT / ".local/sc64/backwater/generated/palette-remap.bin"
DEFAULT_OUT = ROOT / ".local/sc64/interface/generated"
DEFAULT_HEADER = DEFAULT_OUT / "sc64_icons_generated.h"
DEFAULT_REPORT = DEFAULT_OUT / "sc64_icon_asset_report.json"

# The first 228 frames are the stable raw-unit icon binding used by the N64
# command interface. The appended named frames cover rows not keyed by a unit.
SPECIAL_ICONS = (
    ("MOVE", "order", 228, 228),
    ("STOP", "order", 229, 229),
    ("ATTACK", "order", 230, 230),
    ("UNLOAD", "order", 312, 312),
    ("LIFT", "order", 282, 282),
    ("LAND", "order", 283, 283),
    ("TERRAN_WEAPONS", "upgrade", 7, 288),
    ("TERRAN_ARMOR", "upgrade", 0, 292),
    ("ZERG_WEAPONS", "upgrade", 11, 300),
    ("ZERG_ARMOR", "upgrade", 3, 297),
    ("PROTOSS_WEAPONS", "upgrade", 13, 305),
    ("PROTOSS_ARMOR", "upgrade", 5, 303),
    ("VENTRAL_SACS", "upgrade", 24, 261),
    ("CLOAK", "tech", 10, 252),
    ("CONSUME", "tech", 16, 302),
    ("STORM", "tech", 19, 275),
)
UNIT_ICON_COUNT = 228
ICON_COUNT = UNIT_ICON_COUNT + len(SPECIAL_ICONS)


def decode_grp(data: bytes) -> dict:
    """Decode a 4-bit indexed GRP into clipped frame rectangles and alpha."""
    if len(data) < 6:
        raise ValueError("GRP is shorter than its header")
    frame_count = int.from_bytes(data[0:2], "little")
    canvas_w = int.from_bytes(data[2:4], "little")
    canvas_h = int.from_bytes(data[4:6], "little")
    table_end = 6 + frame_count * 8
    if not frame_count or not canvas_w or not canvas_h or table_end > len(data):
        raise ValueError("invalid GRP dimensions or frame table")
    frames = []
    for frame_id in range(frame_count):
        base = 6 + frame_id * 8
        x, y, width, height = data[base:base + 4]
        offset = int.from_bytes(data[base + 4:base + 8], "little")
        if (offset < table_end or offset + height * 2 > len(data) or
                x + width > canvas_w or y + height > canvas_h):
            raise ValueError(f"frame {frame_id}: invalid rectangle or scanline table")
        pixels = bytearray(width * height)
        alpha = bytearray(width * height)
        for row in range(height):
            rel = int.from_bytes(data[offset + row * 2:offset + row * 2 + 2], "little")
            pos = offset + rel
            col = 0
            while col < width:
                if pos >= len(data):
                    raise ValueError(f"frame {frame_id} row {row}: truncated run")
                command = data[pos]
                pos += 1
                if command & 0x80:
                    run, value = command & 0x7F, None
                elif command & 0x40:
                    run, value = command & 0x3F, "repeat"
                else:
                    run, value = command, "literal"
                if not run or run > width - col:
                    raise ValueError(f"frame {frame_id} row {row}: invalid run length")
                start = row * width + col
                if value == "repeat":
                    if pos >= len(data):
                        raise ValueError(f"frame {frame_id} row {row}: missing repeat pixel")
                    pixels[start:start + run] = bytes((data[pos],)) * run
                    alpha[start:start + run] = b"\1" * run
                    pos += 1
                elif value == "literal":
                    if pos + run > len(data):
                        raise ValueError(f"frame {frame_id} row {row}: truncated literal pixels")
                    pixels[start:start + run] = data[pos:pos + run]
                    alpha[start:start + run] = b"\1" * run
                    pos += run
                col += run
        frames.append({"x": x, "y": y, "w": width, "h": height,
                       "pixels": bytes(pixels), "alpha": bytes(alpha)})
    return {"frame_count": frame_count, "canvas_w": canvas_w,
            "canvas_h": canvas_h, "frames": frames}


def crop_frame(frame: dict, remap: bytes) -> bytes:
    """Compact a decoded frame, reserving GBA index 0 for transparent."""
    if len(remap) != 256 or not 32 <= min(remap) <= max(remap) <= 255:
        raise ValueError("world palette remap must map source colors into indices 32..255")
    pixels, alpha = frame["pixels"], frame["alpha"]
    if len(pixels) != len(alpha) or len(pixels) != frame["w"] * frame["h"]:
        raise ValueError("GRP frame buffer dimensions are inconsistent")
    return bytes(remap[color] if alpha[i] and color else 0
                 for i, color in enumerate(pixels))


def fit_icon(frame: dict, remap: bytes, bound: int = 20) -> dict:
    """Palette-map and proportionally fit a GRP icon into the HUD's 20px box."""
    if bound <= 0:
        raise ValueError("icon bound must be positive")
    source = crop_frame(frame, remap)
    width, height = frame["w"], frame["h"]
    if width > bound or height > bound:
        if width >= height:
            target_w = bound
            target_h = max(1, round(height * bound / width))
        else:
            target_h = bound
            target_w = max(1, round(width * bound / height))
        source = scale_indexed(source, width, height, target_w, target_h)
        width, height = target_w, target_h
    return {"w": width, "h": height, "pixels": source}


def scale_indexed(pixels: bytes, width: int, height: int,
                  target_width: int, target_height: int) -> bytes:
    """Nearest-neighbor resize helper used by pixel-oracle tests and preview UI."""
    if min(width, height, target_width, target_height) <= 0 or len(pixels) != width * height:
        raise ValueError("indexed image dimensions do not match the pixel buffer")
    return bytes(pixels[min(height - 1, (y * height) // target_height) * width +
                         min(width - 1, (x * width) // target_width)]
                 for y in range(target_height) for x in range(target_width))


def c_array(name: str, ctype: str, values, width: int = 20) -> str:
    values = list(values)
    return (f"static const {ctype} {name}[{len(values)}] = {{\n" +
            ",\n".join(",".join(str(v) for v in values[i:i + width])
                       for i in range(0, len(values), width)) + "\n};\n")


def build_header(frames: list[dict]) -> str:
    if len(frames) != ICON_COUNT:
        raise ValueError(f"expected {ICON_COUNT} frames, got {len(frames)}")
    total = sum(len(frame["pixels"]) for frame in frames)
    if total > 0xFFFFFFFF:
        raise ValueError("icon pixel table exceeds 32-bit frame offsets")
    if any(not 0 <= frame["w"] <= 255 or not 0 <= frame["h"] <= 255 or
           len(frame["pixels"]) != frame["w"] * frame["h"] for frame in frames):
        raise ValueError("icon frame dimensions or storage are invalid")
    if any(pixel and not 32 <= pixel <= 255 for frame in frames for pixel in frame["pixels"]):
        raise ValueError("icon color indices overlap reserved UI palette entries")
    names = [f"UNIT_{i}" for i in range(UNIT_ICON_COUNT)] + [row[0] for row in SPECIAL_ICONS]
    lines = [
        "/* Generated from a verified private ROM; do not commit this file. */\n",
        "#ifndef SC64_ICONS_GENERATED_H\n#define SC64_ICONS_GENERATED_H\n",
        "#define SC64_HAS_ICON_ASSETS 1\n",
        "enum { SC64_ICON_UNIT_COUNT = 228,\n",
    ]
    lines.extend(f"    SC64_ICON_{name} = {index},\n" for index, name in enumerate(names))
    lines.append(f"    SC64_ICON_COUNT = {ICON_COUNT}\n}};\n")
    lines.append("typedef struct { unsigned int offset; unsigned char width, height; } Sc64IconFrame;\n")
    pixels = bytearray()
    descriptors = []
    deduplicated = {}
    for frame in frames:
        data = frame["pixels"]
        offset = deduplicated.get(data)
        if offset is None:
            offset = len(pixels)
            deduplicated[data] = offset
            pixels.extend(data)
        descriptors.append((offset, frame["w"], frame["h"]))
    lines.append(c_array("sc64_icon_pixels", "unsigned char", pixels))
    lines.append("static const Sc64IconFrame sc64_icons[SC64_ICON_COUNT] = {\n")
    lines.extend(f"    {{{off}, {w}, {h}}},\n" for off, w, h in descriptors)
    lines.extend(["};\n#endif\n"])
    return "".join(lines)


def generate(rom_path: Path, reference: Path, remap_path: Path,
             header_path: Path = DEFAULT_HEADER, report_path: Path = DEFAULT_REPORT) -> dict:
    if not remap_path.is_file():
        raise ValueError(f"world palette remap is missing: {remap_path}")
    remap = remap_path.read_bytes()
    if len(remap) != 256 or min(remap) < 32 or max(remap) > 255:
        raise ValueError("world palette remap must contain 256 indices in range 32..255")
    try:
        commit = subprocess.check_output(["git", "-C", str(reference), "rev-parse", "HEAD"],
                                         text=True).strip()
    except (OSError, subprocess.CalledProcessError) as error:
        raise ValueError("could not verify the pinned references/sc64-maps checkout") from error
    if commit != REFERENCE_COMMIT:
        raise ValueError(f"references/sc64-maps must stay at pinned commit {REFERENCE_COMMIT}")
    sys.path.insert(0, str(reference))
    from extract_sc64_maps import BoltArchive, load_rom

    rom = load_rom(str(rom_path))
    rom_sha1 = hashlib.sha1(rom).hexdigest()
    if rom_sha1 != ROM_SHA1:
        raise ValueError("expected the verified StarCraft 64 USA ROM")
    archive = BoltArchive(rom)
    entries = {entry.path: entry for entry in archive.entries()}
    if ICON_GRP not in entries or WORLD_PALETTE not in entries:
        raise ValueError("verified ROM is missing the command atlas or world palette")
    grp_data = archive.read(entries[ICON_GRP])
    palette_data = archive.read(entries[WORLD_PALETTE])
    if hashlib.sha1(grp_data).hexdigest() != "e64e5ed3f69fb537556f381ac900372cf38ffb9b":
        raise ValueError("command icon atlas differs from the verified ROM asset")
    if len(palette_data) != 512:
        raise ValueError("source world palette must contain 256 RGBA5551 entries")
    grp = decode_grp(grp_data)
    specifications = [(f"UNIT_{unit}", "unit", unit, unit) for unit in range(UNIT_ICON_COUNT)]
    specifications.extend(SPECIAL_ICONS)
    if len(specifications) != ICON_COUNT:
        raise AssertionError("icon source bindings are incomplete")
    frames = []
    manifest_entries = []
    for name, source, source_id, frame_id in specifications:
        if not 0 <= frame_id < grp["frame_count"]:
            raise ValueError(f"{name} references missing atlas frame {frame_id}")
        frame = grp["frames"][frame_id]
        fitted = fit_icon(frame, remap)
        frames.append(fitted)
        manifest_entries.append({"name": name, "source": source, "id": source_id,
                                 "frame": frame_id, "source_w": frame["w"],
                                 "source_h": frame["h"], "w": fitted["w"], "h": fitted["h"],
                                 "opaque_pixels": sum(frame["alpha"])})
    header_path.parent.mkdir(parents=True, exist_ok=True)
    temp_header = header_path.with_suffix(header_path.suffix + ".tmp")
    temp_header.write_text(build_header(frames), encoding="ascii")
    temp_header.replace(header_path)
    report = {"rom_sha1": rom_sha1, "grp": ICON_GRP,
              "grp_sha1": hashlib.sha1(grp_data).hexdigest(),
              "source_palette": WORLD_PALETTE,
              "source_palette_sha1": hashlib.sha1(palette_data).hexdigest(),
              "world_remap_sha1": hashlib.sha1(remap).hexdigest(),
              "reference_commit": commit, "icons": len(frames),
              "pixel_bytes": sum(len(frame["pixels"]) for frame in frames),
              "entries": manifest_entries,
              "generated_header": str(header_path)}
    report_path.parent.mkdir(parents=True, exist_ok=True)
    temp_report = report_path.with_suffix(report_path.suffix + ".tmp")
    temp_report.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    temp_report.replace(report_path)
    return report


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom", type=Path, default=ROOT / ".local/sc64/rom/StarCraft64-USA.z64")
    parser.add_argument("--reference", type=Path, default=ROOT / "references/sc64-maps")
    parser.add_argument("--remap", type=Path, default=DEFAULT_REMAP)
    parser.add_argument("--out", type=Path, default=DEFAULT_OUT)
    args = parser.parse_args()
    header = args.out / "sc64_icons_generated.h"
    report = args.out / "sc64_icon_asset_report.json"
    try:
        result = generate(args.rom, args.reference, args.remap, header, report)
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        parser.error(str(error))
    print(f"Generated {result['icons']} private source icons ({result['pixel_bytes']} pixels).")
    print(f"Header: {header}\nReport: {report}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
