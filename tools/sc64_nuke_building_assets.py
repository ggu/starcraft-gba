#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Prepare the owned StarCraft 64 Nuclear Silo and Covert Ops sprite banks."""
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
from tools import sc64_weapon_source_audit as audit  # noqa: E402
from tools import sc64_weapon_source_sprites as source  # noqa: E402

HEADER = "sc64_nuke_building_source_generated.h"
ASSETS = {
    "silo": {
        "raw": 108, "flingy": 108, "sprite": 265, "canvas": (64, 32),
        "body": (312, 128, 342, 2,
                 "af2c3f10289aa6997e0b2d64dc272e12fa13aa7d2926810bb950edda71c036d8"),
        "landing": (313, 129, 344, 5,
                    "25ca692257e0ca1e12803838c6da5007940d7ed9ecd3c30b616266498c6772da"),
        "working": (314, 130, 345, 1,
                    "75a1c1c8b72ab938f548288eb2625f9b1884dd84885f77b055f1fcaae8f91378"),
    },
    "covert": {
        "raw": 117, "flingy": 98, "sprite": 256, "canvas": (64, 64),
        "body": (288, 113, 307, 2,
                 "f693388a91f20608e22e6e77c192042ba2d3918b43d3f88da376171e1b9fda35"),
        "landing": (289, 114, 309, 5,
                    "83337dd8354158a8fccd96edef1af8f93df9e5952ca8345915b923d422144f63"),
        "working": (290, 115, 310, 1,
                    "9e4f52435d662db29f5830ccf7f7486d56e17e5e7ff38c7ca53e1df92a70d274"),
    },
}


def _build(archive, remap: bytes) -> tuple[str, dict]:
    if len(remap) != 256:
        raise ValueError("palette remap must contain 256 entries")
    refs = sprites.resolve_grp_indices(archive, {name: v["raw"] for name, v in ASSETS.items()})
    images = sprites._entry_payload(archive, "00C/000")
    instructions = sprites._entry_payload(archive, "012/002")
    index = script.script_index(instructions)
    directions, iscripts, groups = attack.image_tables(images)
    modifiers = audit.read_column(images, 999, "B", 999 * 8, "images.modifier")
    shifts = audit.read_column(images, 999, "B", 999 * 9, "images.shift")

    def anim(image: int, name: str) -> list:
        starts = script.image_animation_starts(instructions, index[iscripts[image]])
        return script.animation_instructions(instructions, starts[name])

    lines = [
        "/* Generated from the owned StarCraft 64 ROM. Keep this asset private. */",
        "#ifndef SC64_NUKE_BUILDING_SOURCE_GENERATED_H",
        "#define SC64_NUKE_BUILDING_SOURCE_GENERATED_H",
        "/* Include after Sc64SpriteFrame is declared. */",
        "#define SC64_NUKE_BUILDING_LANDING_HOLD 2u",
        "#define SC64_NUKE_BUILDING_WORKING_HOLD 5u",
    ]
    total = 0
    report = {}
    for name, record in ASSETS.items():
        ref = refs[name]
        body, landing, working = (record[k][0] for k in ("body", "landing", "working"))
        if (ref["flingy_id"], ref["sprite_id"], ref["image_id"]) != (
                record["flingy"], record["sprite"], body):
            raise ValueError(f"{name}: unexpected raw unit mapping")
        # Body and source overlays are normal indexed-color images. The
        # source Init also attaches a destination-dependent shadow, which
        # is not flattened into an opaque body for this shared art bank.
        if anim(body, "Init")[:2] != [(9, [body + 3, 0, 0]), (0, [0])] or modifiers[body + 3] != 10:
            raise ValueError(f"{name}: unexpected source Init")
        if (anim(body, "Built")[0], anim(body, "AlmostBuilt")[0],
                anim(body, "Landing")[0], anim(body, "IsWorking")[0]) != (
                (0, [0]), (0, [1]), (8, [landing, 0, 0]), (8, [working, 0, 0])):
            raise ValueError(f"{name}: unexpected source body stage or overlay mapping")
        landing_steps = [(op, args) for f in range(4)
                         for op, args in ((0, [f]), (5, [2]))] + [(0, [4])]
        if anim(landing, "Init")[:9] != landing_steps:
            raise ValueError(f"{name}: unexpected source landing frame cadence")
        if anim(working, "Init")[:5] != [(0, [0]), (51, []), (5, [5]), (50, []), (5, [5])]:
            raise ValueError(f"{name}: unexpected source working overlay cadence")

        report[name] = {"raw": record["raw"], "sprite": record["sprite"], "images": {}}
        for stage in ("body", "landing", "working"):
            image, iscript, group, count, sha = record[stage]
            path = audit.grp_path(group)
            if not path or (iscripts[image], groups[image], directions[image],
                            modifiers[image], shifts[image]) != (iscript, group, 0, 0, 0):
                raise ValueError(f"{name} {stage}: unexpected source image mapping")
            raw = sprites._entry_payload(archive, path)
            if nuke.digest(raw) != sha:
                raise ValueError(f"{name} {stage}: unexpected source GRP bytes")
            grp = sprites.parse_grp(raw)
            cw, ch = grp["width"], grp["height"]
            if (cw, ch) != record["canvas"] or len(grp["frames"]) != count:
                raise ValueError(f"{name} {stage}: unexpected source canvas or frame count")
            stem = f"sc64_nuke_building_{name}_{stage}"
            rows = []
            for index_in_grp, frame in enumerate(grp["frames"]):
                x, y, w, h = (frame[k] for k in ("x", "y", "frame_width", "frame_height"))
                if not w or not h or x + w > cw or y + h > ch:
                    raise ValueError(f"{name} {stage} frame {index_in_grp}: invalid rectangle")
                alpha = b"".join(frame["alpha"][(y + r) * cw + x:(y + r) * cw + x + w]
                                 for r in range(h))
                data = source.convert_pixels(sprites.crop_frame(frame), alpha, 0, remap)
                symbol = f"{stem}_{index_in_grp}_pixels"
                lines.append(f"static const unsigned char {symbol}[] = {{")
                lines.extend("    " + ",".join(map(str, data[i:i + 24])) + ","
                             for i in range(0, len(data), 24))
                lines.append("};")
                rows.append(f"    {{{x},{y},{w},{h},{cw},{ch},{symbol}}},")
                total += len(data)
            lines.extend([f"static const Sc64SpriteFrame {stem}[] = {{", *rows, "};"])
            report[name]["images"][stage] = {"image": image, "path": path,
                                             "sha256": sha, "frame_count": count}
    if total > 8 * 1024:
        raise ValueError("nuclear building art exceeds the ROM budget")
    return "\n".join([*lines, "#endif", ""]), {"art_bytes": total, "buildings": report}


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
                           ("building_manifest.json", json.dumps(report, indent=2) + "\n")):
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
        print(f"Nuclear building asset preparation: {exc}", file=sys.stderr)
        return 1
    print(f"Prepared {report['art_bytes']} private nuclear building sprite bytes")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
