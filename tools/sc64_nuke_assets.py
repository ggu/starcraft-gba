#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Extract and compress the owned USA StarCraft 64 nuclear strike source art."""
from __future__ import annotations

import argparse
from dataclasses import dataclass
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from tools import sc64_construction_sprites as scripts  # noqa: E402
from tools import sc64_sprites as sprites  # noqa: E402
from tools import sc64_unit_attack_sprites as attack  # noqa: E402
from tools import sc64_weapon_source_audit as audit  # noqa: E402
from tools import sc64_weapon_source_sprites as source  # noqa: E402

DEFAULT_OUT = ROOT / ".local/sc64/nuke/generated"
HEADER = "sc64_nuke_source_generated.h"
EXTRACTOR_SHA256 = "6a84761a9a8074fcc393527a7b17cea194084e8ab49176ff82fa3bef8e1b1dc2"
# Raw missile unit 14; target-dot sprite 231 and impact sprite 267.
# The latter and the missile's small burst are read from missile SpecialState1.
MISSILE_UNIT, TARGET_SPRITE, IMPACT_SPRITE = 14, 231, 267
# Missile's launch/return headings are 0 and 128, the exact ends of the 17
# source directions, as used by order_NukeLaunch.
MISSILE_DIRECTIONS = (0, 16)


@dataclass(frozen=True)
class Image:
    name: str
    image: int
    script: int
    grp: int
    width: int
    height: int
    frames: int
    directions: int
    modifier: int
    shift: int
    sha256: str
    hold: int


IMAGES = (
    Image("missile", 316, 131, 347, 24, 20, 17, 1, 0, 0,
          "1cc02cf6d888e059e0a98b3f89cfc56d00d4fb0709e5d67e8c226b74a3fbc045", 0),
    Image("dot", 233, 74, 238, 4, 4, 2, 0, 0, 0,
          "0ae73158ee5bc60442ffa04f9fa1ba7141eb3cf41881d1c6bfddcf1aebac9040", 1),
    Image("burst", 428, 279, 451, 22, 30, 10, 0, 9, 1,
          "de1991b58ab35defd478f2b54b8013705310c375dbcc22e65baaec4c5be247d2", 2),
    Image("impact", 318, 133, 348, 126, 126, 26, 0, 9, 1,
          "bee335f18f7bbe2629a353a18f68476c63fefd8fa48442a37fb87dd043831a56", 2),
)


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def pack_frame(data: bytes, width: int, height: int) -> bytes:
    """Pack rows as literal/repeat/transparent spans (count 1..63)."""
    if width <= 0 or height <= 0 or len(data) != width * height:
        raise ValueError("frame pixels do not match its rectangle")
    result = bytearray()
    for start in range(0, len(data), width):
        end = start + width
        pos = start
        while pos < end:
            value = data[pos]
            stop = pos + 1
            while stop < min(end, pos + 63) and data[stop] == value:
                stop += 1
            count = stop - pos
            if not value:
                result.append(0x80 | count)
                pos = stop
            elif count >= 3:
                result.extend((0x40 | count, value))
                pos = stop
            else:
                first = pos
                pos = stop
                while pos < min(end, first + 63):
                    if data[pos] == 0 or (pos + 2 < end and
                                         data[pos] == data[pos + 1] == data[pos + 2]):
                        break
                    pos += 1
                result.append(pos - first)
                result.extend(data[first:pos])
    return bytes(result)


def _animation(payload: bytes, index: dict[int, int], image: Image, name: str) -> list:
    starts = scripts.image_animation_starts(payload, index[image.script])
    if name not in starts:
        raise ValueError(f"image {image.image}: missing {name} animation")
    return scripts.animation_instructions(payload, starts[name])


def _validate_animations(payload: bytes) -> None:
    index = scripts.script_index(payload)
    missile = IMAGES[0]
    launch = _animation(payload, index, missile, "SpecialState1")
    if launch[:5] != [(39, []), (36, [1]), (50, []), (8, [428, 0, 0]),
                      (15, [IMPACT_SPRITE, 0, -42])]:
        raise ValueError("missile SpecialState1 no longer selects the expected effects")
    if _animation(payload, index, missile, "Init")[0] != (0, [0]):
        raise ValueError("unexpected missile directional group")
    for image in IMAGES[1:]:
        anim = _animation(payload, index, image, "Init")
        if [args[0] for op, args in anim if op == 0] != list(range(image.frames)):
            raise ValueError(f"image {image.image}: unexpected frame order")
        delays = []
        for op, args in anim:
            if op == 0:
                delays.append(0)
            elif op == 5 and delays:
                delays[-1] += args[0]
            elif op not in (5, 7, 22, 24):
                raise ValueError(f"image {image.image}: unexpected Init opcode {op}")
        if delays != [image.hold] * image.frames:
            raise ValueError(f"image {image.image}: unexpected source timing")
        if anim[-1][0] != (7 if image.name == "dot" else 22):
            raise ValueError(f"image {image.image}: unexpected Init termination")


def _collect(get, remap: bytes, palette: dict) -> tuple[bytes, bytes, dict, dict]:
    if len(remap) != 256:
        raise ValueError("palette remap must contain 256 entries")
    colors = palette.get("gba_rgb555")
    if not isinstance(colors, list) or len(colors) != 256 or any(
            type(v) is not int or not 0 <= v < 0x8000 for v in colors):
        raise ValueError("palette JSON must contain 256 GBA RGB555 colors")

    units = audit.read_column(get("00C/001"), 228, "B", 0, "units.flingy")
    flingy = audit.read_column(get("00C/002"), 209, "H", 0, "flingy.sprite")
    sprite_image = audit.read_column(get("00C/003"), 517, "H", 0, "sprites.image")
    if (units[MISSILE_UNIT], flingy[109], sprite_image[266],
            sprite_image[TARGET_SPRITE], sprite_image[IMPACT_SPRITE]) != (109, 266, 316, 233, 318):
        raise ValueError("unexpected missile, target dot or impact sprite mapping")

    images = get("00C/000")
    directions, iscripts, groups = attack.image_tables(images)
    modifiers = audit.read_column(images, 999, "B", 999 * 8, "images.modifier")
    shifts = audit.read_column(images, 999, "B", 999 * 9, "images.shift")
    _validate_animations(get("012/002"))

    pixels = bytearray()
    rows, manifest = {}, {}
    offsets = {}
    for image in IMAGES:
        i = image.image
        if (iscripts[i], groups[i], directions[i], modifiers[i], shifts[i]) != (
                image.script, image.grp, image.directions, image.modifier, image.shift):
            raise ValueError(f"image {i}: unexpected source mapping")
        path = audit.grp_path(groups[i])
        if path is None:
            raise ValueError(f"image {i}: invalid GRP reference")
        blob = get(path)
        if digest(blob) != image.sha256:
            raise ValueError(f"image {i}: unexpected GRP content")
        grp = sprites.parse_grp(blob)
        cw, ch = grp["width"], grp["height"]
        if (cw, ch, len(grp["frames"])) != (image.width, image.height, image.frames):
            raise ValueError(f"image {i}: unexpected canvas or frame count")
        wanted = MISSILE_DIRECTIONS if image.name == "missile" else tuple(range(image.frames))
        rows[image.name] = []
        uncompressed = 0
        for frame_id in wanted:
            frame = grp["frames"][frame_id]
            x, y, w, h = (frame[k] for k in ("x", "y", "frame_width", "frame_height"))
            if min(x, y) < 0 or not w or not h or x + w > cw or y + h > ch or max(cw, ch) > 255:
                raise ValueError(f"image {i} frame {frame_id}: invalid crop")
            alpha = b"".join(frame["alpha"][(y + r) * cw + x:(y + r) * cw + x + w]
                             for r in range(h))
            raw = source.convert_pixels(sprites.crop_frame(frame), alpha, image.modifier, remap)
            packed = pack_frame(raw, w, h)
            if len(packed) > 65535:
                raise ValueError(f"image {i} frame {frame_id}: packed length exceeds 16 bits")
            if packed not in offsets:
                offsets[packed] = len(pixels)
                pixels.extend(packed)
            rows[image.name].append((offsets[packed], len(packed), x, y, w, h, cw, ch,
                                     image.modifier))
            uncompressed += len(raw)
        manifest[image.name] = {
            "image": i, "grp": path, "grp_sha256": image.sha256,
            "iscript": image.script, "frames": wanted, "modifier": image.modifier,
            "color_shift": image.shift, "ticks_per_frame": image.hold,
            "uncompressed_pixels": uncompressed,
        }
    if len(pixels) > 256 * 1024:
        raise ValueError("packed nuclear frames exceed the ROM budget")
    light_images = {str(i.image): {"modifier": i.modifier, "color_shift": i.shift} for i in IMAGES}
    tables, lut, lut_hashes = source._project_luts(get, palette, light_images)
    if len(tables) != 1 or (tables[0]["modifier"], tables[0]["shift"], tables[0]["rows"]) != (
            9, 1, 63) or len(lut) != 63 * 256:
        raise ValueError("unexpected nuclear light table")
    return bytes(pixels), lut, rows, {"images": manifest, "light_sources_sha256": lut_hashes}


def _header(pixels: bytes, lut: bytes, rows: dict) -> str:
    lines = [
        "/* Generated from the owned StarCraft 64 ROM. Keep this asset private. */",
        "#ifndef SC64_NUKE_SOURCE_GENERATED_H", "#define SC64_NUKE_SOURCE_GENERATED_H",
        "#include <stdint.h>",
        "/* Row spans: low 6 bits count; high bits 00 literal, 01 repeat, 10 skip. */",
        "typedef struct {",
        "    uint32_t offset; uint16_t length;",
        "    uint8_t x,y,w,h,cw,ch,modifier;",
        "} Sc64NukeFrame;",
        "#define SC64_NUKE_LIGHT_ROWS 63",
        "#define SC64_NUKE_IMPACT_SOURCE_Y (-42)",
        "#define SC64_NUKE_DOT_SOURCE_TICKS 1",
        "#define SC64_NUKE_BURST_SOURCE_TICKS 2",
        "#define SC64_NUKE_IMPACT_SOURCE_TICKS 2",
    ]
    for name, data in (("sc64_nuke_pixels", pixels), ("sc64_nuke_light_lut", lut)):
        lines.append(f"static const uint8_t {name}[] = {{")
        lines.extend("    " + ",".join(map(str, data[i:i + 24])) + ","
                     for i in range(0, len(data), 24))
        lines.append("};")
    for name, data in rows.items():
        lines.append(f"static const Sc64NukeFrame sc64_nuke_{name}_frames[] = {{")
        lines.extend("    {" + ",".join(map(str, row)) + "}," for row in data)
        lines.append("};")
        lines.append(f"#define SC64_NUKE_{name.upper()}_FRAME_COUNT {len(data)}")
    return "\n".join([*lines, "#endif", ""])


def generate(rom: Path, extractor_path: Path, remap_path: Path, palette_path: Path,
             out: Path) -> dict:
    if not out.resolve().is_relative_to((ROOT / ".local").resolve()):
        raise ValueError("generated source art must be written under this worktree's .local")
    if digest(extractor_path.read_bytes()) != EXTRACTOR_SHA256:
        raise ValueError("BOLT extractor does not match the pinned source")
    extractor = sprites.load_extractor(extractor_path)
    data = extractor.load_rom(str(rom))
    rom_sha1 = hashlib.sha1(data).hexdigest()
    if rom_sha1 != sprites.EXPECTED_USA_ROM_SHA1:
        raise ValueError("ROM does not match the verified USA source")
    archive = extractor.BoltArchive(data)
    entries = {}
    for entry in archive.entries():
        if entry.path in entries:
            raise ValueError(f"ambiguous BOLT entry: {entry.path}")
        entries[entry.path] = entry

    def get(path: str) -> bytes:
        if path not in entries:
            raise ValueError(f"BOLT entry is missing: {path}")
        return archive.read(entries[path])

    remap = remap_path.read_bytes()
    palette_bytes = palette_path.read_bytes()
    palette = json.loads(palette_bytes)
    if not isinstance(palette, dict):
        raise ValueError("palette JSON must be an object")
    pixels, lut, rows, manifest = _collect(get, remap, palette)
    header = _header(pixels, lut, rows)
    manifest.update({
        "rom_sha1": rom_sha1, "extractor_sha256": EXTRACTOR_SHA256,
        "palette_sha256": digest(palette_bytes), "remap_sha256": digest(remap),
        "packed_bytes": len(pixels), "light_lut_bytes": len(lut),
        "header_sha256": digest(header.encode("ascii")),
        "scope": "verified source frames and IScript cadence; native screen rendering unmeasured",
    })
    out.mkdir(parents=True, exist_ok=True)
    for name, text in ((HEADER, header), ("manifest.json", json.dumps(manifest, indent=2) + "\n")):
        temp = out / (name + ".tmp")
        temp.write_text(text, encoding="ascii")
        temp.replace(out / name)
    return manifest


def main(argv: list[str] | None = None) -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--rom", type=Path, default=sprites.DEFAULT_ROM)
    p.add_argument("--extractor", type=Path, default=sprites.UPSTREAM)
    p.add_argument("--palette-remap", type=Path, default=source.DEFAULT_REMAP)
    p.add_argument("--palette-json", type=Path, default=source.DEFAULT_PALETTE)
    p.add_argument("--out", type=Path, default=DEFAULT_OUT)
    a = p.parse_args(argv)
    try:
        manifest = generate(a.rom, a.extractor, a.palette_remap, a.palette_json, a.out)
    except (OSError, ValueError, KeyError) as exc:
        print(f"Nuclear asset preparation: {exc}", file=sys.stderr)
        return 1
    print(f"Prepared {manifest['packed_bytes']} private nuclear sprite bytes "
          f"and a {manifest['light_lut_bytes']}-byte light table")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
