#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Import the owned StarCraft 64 Ghost's Nuclear Strike painting poses."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from tools import sc64_construction_sprites as script  # noqa: E402
from tools import sc64_nuke_assets as nuke  # noqa: E402
from tools import sc64_sprites as sprites  # noqa: E402
from tools import sc64_unit_attack_sprites as attack  # noqa: E402
from tools import sc64_weapon_source_sprites as source  # noqa: E402

HEADER = "sc64_nuke_ghost_source_generated.h"
RAW, FLINGY, SPRITE, IMAGE, ISCRIPT, GRP = 1, 74, 229, 228, 70, 234
GRP_PATH = "00D/0E9"
GRP_SHA256 = "8af573b8cd2af4bafd6ed68e2ce12120893eb69bceb68804d86c4e93d3f03dac"
BASES = (0, 17, 34)


def _build(archive, remap: bytes) -> tuple[str, dict]:
    if len(remap) != 256:
        raise ValueError("palette remap must contain 256 entries")
    ref = sprites.resolve_grp_indices(archive, {"ghost": RAW})["ghost"]
    if (ref["flingy_id"], ref["sprite_id"], ref["image_id"],
            ref["grp_filename_index"], ref["bolt_path"]) != (FLINGY, SPRITE, IMAGE, GRP, GRP_PATH):
        raise ValueError("unexpected Ghost raw unit mapping")
    images = sprites._entry_payload(archive, "00C/000")
    directions, iscripts, groups = attack.image_tables(images)
    if (directions[IMAGE], iscripts[IMAGE], groups[IMAGE],
            images[999 * 8 + IMAGE], images[999 * 9 + IMAGE]) != (1, ISCRIPT, GRP, 0, 0):
        raise ValueError("unexpected Ghost image mapping")
    payload = sprites._entry_payload(archive, "012/002")
    index = script.script_index(payload)
    start = script.image_animation_starts(payload, index[ISCRIPT])["SpecialState1"]
    # Two transitions, then the third frame holds through the shared idle
    # wait/goto loop. This is the Ghost's paint pose, not its weapon attack.
    expected = [(op, args) for base in BASES for op, args in ((0, [base]), (5, [1]))]
    expected += [(7, [31162]), (5, [125]), (7, [31162])]
    if script.animation_instructions(payload, start) != expected:
        raise ValueError("unexpected Ghost painting frame sequence")
    blob = sprites._entry_payload(archive, GRP_PATH)
    if nuke.digest(blob) != GRP_SHA256:
        raise ValueError("unexpected Ghost GRP source content")
    grp = sprites.parse_grp(blob)
    if (grp["width"], grp["height"], len(grp["frames"])) != (32, 32, 229):
        raise ValueError("unexpected Ghost GRP canvas or frame count")

    pixels = bytearray()
    offsets = {}
    rows = []
    for base in BASES:
        group = []
        for dir_ in range(17):
            frame = grp["frames"][attack.resolve_frame_index(base, dir_, True, len(grp["frames"]))]
            packed = attack.map_frame(frame, remap)
            key = packed["pixels"]
            if sum(value != 0 for value in key) != sum(frame["alpha"]):
                raise ValueError("Ghost paint remap does not preserve the source alpha mask")
            if key not in offsets:
                offsets[key] = len(pixels)
                pixels.extend(key)
            group.append(tuple(packed[k] for k in ("offset_x", "offset_y", "w", "h", "canvas_w",
                                                   "canvas_h")) + (offsets[key],))
        rows.append(group)
    if len(pixels) > 4 * 1024:
        raise ValueError("Ghost painting art exceeds the pixel budget")
    lines = [
        "/* Generated from the owned StarCraft 64 ROM. Keep this asset private. */",
        "#ifndef SC64_NUKE_GHOST_SOURCE_GENERATED_H",
        "#define SC64_NUKE_GHOST_SOURCE_GENERATED_H",
        "/* Include after Sc64SpriteFrame is declared. */",
        "static const unsigned char sc64_nuke_ghost_paint_pixels[] = {",
        *("    " + ",".join(map(str, pixels[i:i + 24])) + ","
          for i in range(0, len(pixels), 24)),
        "};",
        "static const Sc64SpriteFrame sc64_nuke_ghost_paint_frames[3][17] = {",
    ]
    for row in rows:
        lines.append("    {")
        lines.extend("        {" + ",".join(map(str, frame[:-1])) +
                     f",sc64_nuke_ghost_paint_pixels+{frame[-1]}}}," for frame in row)
        lines.append("    },")
    lines.extend(["};", "#endif", ""])
    return "\n".join(lines), {
        "raw": RAW, "flingy": FLINGY, "sprite": SPRITE, "image": IMAGE, "iscript": ISCRIPT,
        "grp": GRP_PATH, "grp_sha256": GRP_SHA256, "directional_bases": BASES,
        "holds": [1, 1, "until tracking ends"], "directions_per_base": 17,
        "distinct_payloads": len(offsets), "art_bytes": len(pixels),
    }


def generate(rom: Path, extractor_path: Path, remap_path: Path, out: Path) -> dict:
    if not out.resolve().is_relative_to((ROOT / ".local").resolve()):
        raise ValueError("generated source art must stay under this worktree's .local")
    if nuke.digest(extractor_path.read_bytes()) != nuke.EXTRACTOR_SHA256:
        raise ValueError("BOLT extractor does not match the pinned source")
    extractor = sprites.load_extractor(extractor_path)
    data = extractor.load_rom(str(rom))
    if hashlib.sha1(data).hexdigest() != sprites.EXPECTED_USA_ROM_SHA1:
        raise ValueError("ROM does not match the verified USA source")
    remap = remap_path.read_bytes()
    header, report = _build(extractor.BoltArchive(data), remap)
    report.update(header_sha256=nuke.digest(header.encode("ascii")),
                  remap_sha256=nuke.digest(remap))
    out.mkdir(parents=True, exist_ok=True)
    for name, contents in ((HEADER, header),
                           ("ghost_manifest.json", json.dumps(report, indent=2) + "\n")):
        temp = out / (name + ".tmp")
        temp.write_text(contents, encoding="ascii")
        temp.replace(out / name)
    return report


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom", type=Path, default=sprites.DEFAULT_ROM)
    parser.add_argument("--extractor", type=Path, default=sprites.UPSTREAM)
    parser.add_argument("--palette-remap", type=Path, default=source.DEFAULT_REMAP)
    parser.add_argument("--out", type=Path, default=nuke.DEFAULT_OUT)
    args = parser.parse_args()
    try:
        report = generate(args.rom, args.extractor, args.palette_remap, args.out)
    except (OSError, ValueError, KeyError) as exc:
        print(f"Ghost painting asset preparation: {exc}", file=sys.stderr)
        return 1
    print(f"Prepared {report['art_bytes']} private Ghost painting sprite bytes")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
