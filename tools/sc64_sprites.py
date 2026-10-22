#!/usr/bin/env python3
"""Decode selected StarCraft 64 unit GRPs from a user-supplied USA ROM.

Only indexed source pixels, their N64 palette, and an audit manifest are
written, all below .local/. Keep those derived assets private.

Copyright (C) 2026 blizzard-gba contributors
SPDX-License-Identifier: GPL-3.0-or-later
"""

from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import struct
import sys
import zlib
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
UPSTREAM = ROOT / "references/sc64-maps/extract_sc64_maps.py"
UPSTREAM_COMMIT = "09b57f787a247107d1de82b8c5eab2734667ea1d"
EXPECTED_USA_ROM_SHA1 = "a36d91b5aac7bb95df0d7de5f9814f5270e25c2b"
DEFAULT_ROM = ROOT / ".local/sc64/rom/StarCraft64-USA.z64"
DEFAULT_OUT = ROOT / ".local/sc64/generated/sprites"
DEFAULT_REMAP = ROOT / ".local/sc64/generated/palette-remap.bin"
DEFAULT_HEADER = ROOT / ".local/sc64/generated/sc64_sprites_generated.h"
BACKWATER_OUT = ROOT / ".local/sc64/backwater/generated/sprites"
BACKWATER_HEADER = ROOT / ".local/sc64/backwater/generated/sc64_sprites_generated.h"
BACKWATER_REMAP = ROOT / ".local/sc64/backwater/generated/palette-remap.bin"

# These are the actual StarCraft unit ids in the Wasteland CHK, plus the
# unbuilt Barracks goal referenced by its victory trigger.
UNIT_IDS = {
    "marine": 0,
    "scv": 7,
    "raynor_vulture": 19,
    "zergling": 37,
    "command_center": 106,
    "supply_depot": 109,
    "barracks": 111,
    "mineral_field_1": 176,
    "mineral_field_2": 177,
    "mineral_field_3": 178,
}
BACKWATER_UNIT_IDS = {
    **UNIT_IDS,
    "firebat": 32,
    "hydralisk": 38,
    "academy": 112,
    "bunker": 125,
    "hive": 133,
    "extractor": 149,
    "geyser": 188,
    "infested_command_center": 130,
    "zerg_hatchery": 131,
    "zerg_lair": 132,
    "creep_colony": 143,
    "mineral_field_1": 176,
    "mineral_field_2": 177,
    "mineral_field_3": 178,
}
UNIT_COUNT = 228
FLINGY_COUNT = 209
SPRITE_COUNT = 517
IMAGE_COUNT = 999


def load_extractor(path: Path):
    spec = importlib.util.spec_from_file_location("sc64_upstream_sprites", path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load upstream extractor: {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def _entry_payload(archive, path: str) -> bytes:
    for entry in archive.entries():
        if entry.path == path:
            return archive.read(entry)
    raise ValueError(f"BOLT entry is missing: {path}")


def _le_array(data: bytes, count: int, width: int, label: str) -> tuple[int, ...]:
    needed = count * width
    if len(data) < needed:
        raise ValueError(f"{label}: expected at least {needed} bytes, got {len(data)}")
    fmt = {1: "B", 2: "H", 4: "I"}[width]
    return struct.unpack_from("<" + fmt * count, data)


def resolve_grp_indices(archive, unit_ids: dict[str, int] = UNIT_IDS) -> dict[str, dict[str, int | str]]:
    """Follow units.dat -> flingy.dat -> sprites.dat -> images.dat -> BOLT GRP."""
    unit_flingy = _le_array(_entry_payload(archive, "00C/001"), UNIT_COUNT, 1, "units.dat")
    flingy_sprite = _le_array(_entry_payload(archive, "00C/002"), FLINGY_COUNT, 2, "flingy.dat")
    sprite_image = _le_array(_entry_payload(archive, "00C/003"), SPRITE_COUNT, 2, "sprites.dat")
    image_grp = _le_array(_entry_payload(archive, "00C/000"), IMAGE_COUNT, 4, "images.dat")

    result = {}
    for name, unit_id in unit_ids.items():
        if unit_id >= UNIT_COUNT:
            raise ValueError(f"unit id {unit_id} out of range for units.dat")
        flingy_id = unit_flingy[unit_id]
        if flingy_id >= FLINGY_COUNT:
            raise ValueError(f"unit {name}: invalid flingy id {flingy_id}")
        sprite_id = flingy_sprite[flingy_id]
        if sprite_id >= SPRITE_COUNT:
            raise ValueError(f"unit {name}: invalid sprite id {sprite_id}")
        image_id = sprite_image[sprite_id]
        if image_id >= IMAGE_COUNT:
            raise ValueError(f"unit {name}: invalid image id {image_id}")
        gfi = image_grp[image_id]
        if gfi == 0:
            raise ValueError(f"unit {name}: no GRP filename index")
        # OpenBW's images.tbl lookup is 1-based. In the N64 BOLT archive the
        # same 1-based image index selects leaf entries across dirs 00D..011.
        flat = gfi - 1
        directory = 0x00D + flat // 256
        if directory > 0x011:
            raise ValueError(f"unit {name}: GRP index {gfi} outside unit asset bank")
        leaf = flat % 256
        result[name] = {
            "unit_id": unit_id, "flingy_id": flingy_id, "sprite_id": sprite_id,
            "image_id": image_id, "grp_filename_index": gfi,
            "bolt_path": f"{directory:03X}/{leaf:03X}",
        }
    return result


def parse_grp(data: bytes) -> dict[str, Any]:
    """Decode a Blizzard GRP file into indexed full-canvas frames."""
    if len(data) < 6:
        raise ValueError("GRP is shorter than its header")
    frame_count, width, height = struct.unpack_from("<HHH", data)
    header_end = 6 + frame_count * 8
    if not frame_count or not width or not height or header_end > len(data):
        raise ValueError("invalid GRP dimensions/frame table")
    frames = []
    for frame_id in range(frame_count):
        frame_off = 6 + frame_id * 8
        x, y, fw, fh, data_off = struct.unpack_from("<BBBBI", data, frame_off)
        if data_off < header_end or data_off + 2 * fh > len(data):
            raise ValueError(f"frame {frame_id}: invalid line table offset")
        if x + fw > width or y + fh > height:
            raise ValueError(f"frame {frame_id}: frame rectangle exceeds canvas")
        line_table = data_off
        indexed = bytearray(width * height)
        alpha = bytearray(width * height)
        for row in range(fh):
            relative = struct.unpack_from("<H", data, line_table + row * 2)[0]
            pos = data_off + relative
            col = 0
            while col < fw:
                if pos >= len(data):
                    raise ValueError(f"frame {frame_id} row {row}: truncated RLE")
                op = data[pos]
                pos += 1
                if op & 0x80:
                    run = op & 0x7F
                    color = None
                elif op & 0x40:
                    run = op & 0x3F
                    if pos >= len(data):
                        raise ValueError(f"frame {frame_id} row {row}: missing repeat color")
                    color = data[pos]
                    pos += 1
                else:
                    run = op
                    color = "literal"
                if run == 0:
                    raise ValueError(f"frame {frame_id} row {row}: zero-length run")
                if run > fw - col:
                    raise ValueError(f"frame {frame_id} row {row}: RLE exceeds frame width")
                target = (y + row) * width + x + col
                if color is None:
                    pass  # transparent pixels retain alpha=0
                elif color == "literal":
                    if pos + run > len(data):
                        raise ValueError(f"frame {frame_id} row {row}: truncated literal run")
                    indexed[target:target + run] = data[pos:pos + run]
                    alpha[target:target + run] = b"\x01" * run
                    pos += run
                else:
                    indexed[target:target + run] = bytes((color,)) * run
                    alpha[target:target + run] = b"\x01" * run
                col += run
        frames.append({"pixels": bytes(indexed), "alpha": bytes(alpha),
                       "x": x, "y": y, "width": width, "height": height,
                       "frame_width": fw, "frame_height": fh})
    return {"width": width, "height": height, "frames": frames}


def decode_rgba5551_palette(data: bytes) -> list[tuple[int, int, int, int]]:
    if len(data) != 512:
        raise ValueError(f"Badlands unit palette must be 512 bytes, got {len(data)}")
    colors = []
    for i in range(256):
        value = struct.unpack_from(">H", data, i * 2)[0]
        r, g, b, alpha = (value >> 11) & 31, (value >> 6) & 31, (value >> 1) & 31, value & 1
        widen = lambda c: (c << 3) | (c >> 2)
        colors.append((widen(r), widen(g), widen(b), 255 if alpha else 0))
    return colors


def encode_png(path: Path, width: int, height: int, rgba: bytes) -> None:
    if len(rgba) != width * height * 4:
        raise ValueError("PNG pixel buffer has the wrong size")
    rows = bytearray()
    stride = width * 4
    for y in range(height):
        rows.append(0)
        rows += rgba[y * stride:(y + 1) * stride]
    def chunk(kind: bytes, body: bytes) -> bytes:
        return (struct.pack(">I", len(body)) + kind + body
                + struct.pack(">I", zlib.crc32(kind + body) & 0xFFFFFFFF))
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(b"\x89PNG\r\n\x1a\n"
        + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0))
        + chunk(b"IDAT", zlib.compress(bytes(rows), 9)) + chunk(b"IEND", b""))


def render_frame(frame: dict[str, Any], palette: list[tuple[int, int, int, int]]) -> bytes:
    rgba = bytearray(len(frame["pixels"]) * 4)
    for i, color_index in enumerate(frame["pixels"]):
        if frame["alpha"][i]:
            rgba[i * 4:i * 4 + 4] = bytes(palette[color_index])
    return bytes(rgba)


def crop_frame(frame: dict[str, Any]) -> bytes:
    """Return the GRP frame rectangle, retaining zero for transparent pixels."""
    width, height = frame["width"], frame["height"]
    x, y, fw, fh = frame["x"], frame["y"], frame["frame_width"], frame["frame_height"]
    pixels = frame["pixels"]
    return b"".join(pixels[(y + row) * width + x:(y + row) * width + x + fw]
                    for row in range(fh))


def write_c_header(path: Path, sprites: list[dict[str, Any]], remap: bytes) -> None:
    if len(remap) != 256:
        raise ValueError(f"palette remap must contain 256 bytes, got {len(remap)}")
    lines = ["/* Generated from the verified USA ROM; do not commit this file. */",
             "#ifndef SC64_SPRITES_GENERATED_H", "#define SC64_SPRITES_GENERATED_H", "",
             "typedef struct { s16 offset_x, offset_y; u16 w, h, canvas_w, canvas_h; const u8 *pixels; } Sc64SpriteFrame;", ""]
    for sprite in sprites:
        name, frame = sprite["name"], sprite["frame"]
        pixels = bytearray(crop_frame(frame))
        alpha = frame["alpha"]
        x, y, fw = frame["x"], frame["y"], frame["frame_width"]
        for row in range(frame["frame_height"]):
            for col in range(fw):
                source_i = (y + row) * frame["width"] + x + col
                i = row * fw + col
                pixels[i] = remap[pixels[i]] if alpha[source_i] else 0
        symbol = f"sc64_sprite_{name}"
        lines.append(f"static const u8 {symbol}_pixels[{len(pixels)}] = {{")
        for i in range(0, len(pixels), 16):
            lines.append("    " + ", ".join(f"0x{v:02x}" for v in pixels[i:i + 16]) + ",")
        lines += ["};", f"static const Sc64SpriteFrame {symbol} = "
                  f"{{ {frame['x']}, {frame['y']}, {frame['frame_width']}, {frame['frame_height']}, "
                  f"{frame['width']}, {frame['height']}, {symbol}_pixels }};", ""]
    lines += ["#endif", ""]
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines), encoding="ascii")


def extract(rom_path: Path, out_dir: Path, upstream_path: Path = UPSTREAM,
            remap_path: Path = DEFAULT_REMAP, header_path: Path = DEFAULT_HEADER,
            unit_ids: dict[str, int] = UNIT_IDS) -> dict[str, Any]:
    extractor = load_extractor(upstream_path)
    rom_raw = rom_path.read_bytes()
    rom = extractor.load_rom(str(rom_path))
    rom_sha1 = hashlib.sha1(rom).hexdigest()
    if rom_sha1 != EXPECTED_USA_ROM_SHA1:
        raise ValueError(
            "ROM must be the verified USA retail image "
            f"(normalized SHA1 {EXPECTED_USA_ROM_SHA1}; got {rom_sha1})"
        )
    archive = extractor.BoltArchive(rom)
    assets = resolve_grp_indices(archive, unit_ids)
    palette_data = _entry_payload(archive, "00B/004")
    palette = decode_rgba5551_palette(palette_data)
    remap = remap_path.read_bytes()
    if len(remap) != 256:
        raise ValueError(f"palette remap must contain 256 bytes, got {len(remap)}")
    inventory = []
    selected = []
    for name, reference in assets.items():
        bolt_path = reference["bolt_path"]
        entry = next((e for e in archive.entries() if e.path == bolt_path), None)
        if entry is None:
            raise ValueError(f"GRP BOLT entry is missing: {bolt_path}")
        grp_data = archive.read(entry)
        grp = parse_grp(grp_data)
        # Use frame 0: the first facing pose is the standing/idle sprite.
        # Choosing by opaque area can select death or attack frames.
        frame_id, frame = 0, grp["frames"][0]
        target = out_dir / f"{name}.png"
        encode_png(target, grp["width"], grp["height"], render_frame(frame, palette))
        selected.append({"name": name, "frame": frame})
        inventory.append({
            **reference,
            "asset_sha1": hashlib.sha1(grp_data).hexdigest(),
            "grp_size": len(grp_data), "canvas_width": grp["width"],
            "canvas_height": grp["height"], "frame_count": len(grp["frames"]),
            "rendered_frame": frame_id, "frame_x": frame["x"], "frame_y": frame["y"],
            "frame_width": frame["frame_width"], "frame_height": frame["frame_height"],
            "opaque_pixels": sum(frame["alpha"]), "frame_selection": "first GRP frame (standing pose)",
            "preview": target.name,
        })
    manifest = {
        "source": {"rom_filename": rom_path.name, "rom_sha1": rom_sha1,
                   "rom_input_sha1": hashlib.sha1(rom_raw).hexdigest(),
                   "upstream": "references/sc64-maps/extract_sc64_maps.py",
                   "upstream_commit": UPSTREAM_COMMIT},
        "lookup": "units.dat->flingy.dat->sprites.dat->images.dat; 1-based gfi maps to BOLT leaf index gfi-1 across 00D..011",
        "palette": {"bolt_path": "00B/004", "format": "256 big-endian RGBA5551 words",
                    "sha1": hashlib.sha1(palette_data).hexdigest()},
        "assets": inventory,
    }
    out_dir.mkdir(parents=True, exist_ok=True)
    (out_dir / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    write_c_header(header_path, selected, remap)
    manifest["generated_header"] = str(header_path)
    (out_dir / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    return manifest


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom", type=Path, default=DEFAULT_ROM)
    parser.add_argument("--out", type=Path, default=DEFAULT_OUT)
    parser.add_argument("--upstream", type=Path, default=UPSTREAM)
    parser.add_argument("--remap", type=Path, default=DEFAULT_REMAP)
    parser.add_argument("--header", type=Path, default=DEFAULT_HEADER)
    parser.add_argument("--profile", choices=("wasteland", "backwater"), default="wasteland")
    args = parser.parse_args(argv)
    if not args.rom.is_file():
        parser.error(f"ROM does not exist: {args.rom}")
    try:
        if args.profile == "backwater":
            out = args.out if args.out != DEFAULT_OUT else BACKWATER_OUT
            remap = args.remap if args.remap != DEFAULT_REMAP else BACKWATER_REMAP
            header = args.header if args.header != DEFAULT_HEADER else BACKWATER_HEADER
            result = extract(args.rom, out, args.upstream, remap, header, BACKWATER_UNIT_IDS)
        else:
            out = args.out
            result = extract(args.rom, args.out, args.upstream, args.remap, args.header)
    except (OSError, ValueError, RuntimeError) as exc:
        print(f"sc64_sprites: {exc}", file=sys.stderr)
        return 1
    print(f"Decoded {len(result['assets'])} sprites into {out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
