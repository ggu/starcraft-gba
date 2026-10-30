#!/usr/bin/env python3
"""Pack authenticated StarCraft 64 building damage fire into an ignored header.

The pinned OpenBW bwenums and update_unit_damage_overlay identify image IDs
450-457 and 472-479 as small/large orange damage overlays.  Three distinct
GRPs supply the type variants.  The N64 IScript determines their frame loops.
The original draw function blends intensities; Mode 4 uses an opaque ramp.

Copyright (C) 2026 blizzard-gba contributors
SPDX-License-Identifier: GPL-3.0-or-later
"""
from __future__ import annotations
import argparse
import hashlib
import json
import sys
import struct
from pathlib import Path
from typing import Sequence

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from tools import sc64_construction_sprites as cs
from tools import sc64_death_sprites as death
from tools import sc64_sprites as base

IMAGE_IDS = (450, 451, 452, 472, 473, 474, 466, 467, 468, 488, 489, 490, 458, 459, 460, 461, 480, 481, 482, 483)
EXPECTED_PATHS = ('00E/0D3', '00E/0D4', '00E/0D5') * 4 + ('00E/0D6', '00E/0D7', '00E/0D8', '00E/0D9') * 2

def timeline(payload: bytes, start: int) -> list[tuple[int, int]]:
    """Accept only a simple frame/wait loop and reject all other programs."""
    pc = start
    seen = set()
    frame = None
    result = []
    for _ in range(128):
        if pc in seen:
            if pc != start or not result:
                raise ValueError('unsupported noncanonical fire loop')
            return result
        seen.add(pc)
        op, args, next_pc, jump = cs.decode_instruction(payload, pc)
        if op == 0:
            frame = args[0]
        elif op == 5 and frame is not None and args[0] > 0:
            result.append((frame, args[0]))
        elif op == 7 and jump is not None:
            next_pc = jump
        else:
            raise ValueError(f'unsupported fire IScript opcode {op}')
        pc = next_pc
    raise ValueError('fire IScript exceeds instruction bound')

def parse_damage_lo(data: bytes) -> list[tuple[int, int, int]]:
    """Read frame-zero damage anchors; preserve their source slot indices."""
    if len(data) < 12:
        raise ValueError('truncated damage LO')
    frames, count = struct.unpack_from('<II', data)
    directory_end = 8 + frames * 4
    if not frames or count != 22 or directory_end > len(data):
        raise ValueError('invalid damage LO dimensions')
    pos = struct.unpack_from('<I', data, 8)[0]
    if pos < directory_end or pos + count * 2 > len(data):
        raise ValueError('invalid damage LO frame')
    result = []
    for slot in range(count):
        x, y = struct.unpack_from('<bb', data, pos + 2 * slot)
        if (x, y) != (127, 127):
            result.append((x, y, slot))
    return result

def generate(rom_path: Path, remap_path: Path, header_path: Path, out_dir: Path, upstream: Path) -> dict:
    extractor = base.load_extractor(upstream)
    rom = extractor.load_rom(str(rom_path))
    sha = hashlib.sha1(rom).hexdigest()
    if sha != cs.EXPECTED_ROM_SHA1:
        raise ValueError('ROM is not authenticated USA retail image')
    archive = extractor.BoltArchive(rom)
    entries = {e.path: e for e in archive.entries()}
    def read(path: str) -> bytes:
        if path not in entries:
            raise ValueError(f'missing BOLT entry {path}')
        return archive.read(entries[path])
    tables = {}
    for path, expected in cs.TABLE_HASHES.items():
        data = read(path)
        if hashlib.sha256(data).hexdigest() != expected:
            raise ValueError(f'authenticated BOLT table hash mismatch: {path}')
        tables[path] = data
    unit_flingy, _, flingy_sprite, sprite_image, grps, scripts_by_image = cs.table_images(tables['00C/001'], tables['00C/002'], tables['00C/003'], tables['00C/000'])
    payload = tables['012/002']
    scripts = cs.script_index(payload)
    remap = remap_path.read_bytes()
    if len(remap) != 256:
        raise ValueError('palette remap must contain 256 bytes')
    ui = death.ui_palette_indices()
    dat = tables['00C/000']
    images = []
    for iid, path in zip(IMAGE_IDS, EXPECTED_PATHS):
        drawfn = 0 if 458 <= iid <= 465 or 480 <= iid <= 487 else 9
        remap_id = 4 if 466 <= iid <= 471 or 488 <= iid <= 493 else (1 if drawfn else 0)
        if cs.grp_path(grps[iid]) != path or dat[cs.IMAGE_COUNT * 8 + iid] != drawfn or dat[cs.IMAGE_COUNT * 9 + iid] != remap_id:
            raise ValueError(f'image {iid} does not match expected orange overlay')
        start = cs.image_animation_starts(payload, scripts[scripts_by_image[iid]])['Init']
        steps = timeline(payload, start)
        count = 6 if drawfn == 0 else 12
        first = count if iid >= 472 else 0
        expected_frames = list(range(first, first + count))
        if steps != [(f, 2) for f in expected_frames]:
            raise ValueError(f'image {iid} has unexpected IScript timing')
        data = read(path)
        grp = base.parse_grp(data)
        if max(expected_frames) >= len(grp['frames']):
            raise ValueError('fire frame outside GRP')
        frames = death.convert_frames(grp, expected_frames, drawfn, remap_id, remap, death.ramp_indices(remap_id, ui) if drawfn else [])
        for f in frames:
            if not (0 < f['w'] <= 64 and 0 < f['h'] <= 64 and f['canvas_w'] <= 64 and f['canvas_h'] <= 64):
                raise ValueError('fire frame dimensions exceed renderer bounds')
        images.append({'id': iid, 'grp': path, 'sha1': hashlib.sha1(data).hexdigest(), 'frames': frames})
    lines = ['/* Private generated source art. Do not commit. */', '#ifndef SC64_BUILDING_FIRE_GENERATED_H', '#define SC64_BUILDING_FIRE_GENERATED_H', '#define SC64_BUILDING_FIRE_BANK_VERSION 1u']
    for bank, im in enumerate(images):
        for index, f in enumerate(im['frames']):
            sym = f'sc64_building_fire_{bank}_{index}'
            lines.append(f'static const unsigned char {sym}[] = {{')
            for off in range(0, len(f['pixels']), 16):
                lines.append('    ' + ', '.join(str(v) for v in f['pixels'][off:off+16]) + ',')
            lines.append('};')
        lines.append(f"static const Sc64BuildingFireFrame sc64_building_fire_frames_{bank}[{len(im['frames'])}] = {{")
        for index, f in enumerate(im['frames']):
            lines.append(f"    {{ {f['x']}, {f['y']}, {f['w']}, {f['h']}, {f['canvas_w']}, {f['canvas_h']}, sc64_building_fire_{bank}_{index} }},")
        lines.append('};')
    lines.append('static const Sc64BuildingFireFrame *const sc64_building_fire_banks[20] = {')
    lines.append('    ' + ', '.join(f'sc64_building_fire_frames_{i}' for i in range(20)) + ' };')
    # Frame-zero damage LO coordinates.  Slot number selects the source image;
    # (127,127) means absent. Keep only valid entries to bound runtime work.
    anchors = []
    actors = []
    lo_hashes = {}
    damage_column = cs.IMAGE_COUNT * 14 + cs.IMAGE_COUNT * 4 * 2
    for raw in range(106, 176):
        if unit_flingy[raw] >= len(flingy_sprite):
            continue
        sprite = flingy_sprite[unit_flingy[raw]]
        if sprite >= len(sprite_image):
            continue
        iid = sprite_image[sprite]
        gfi = struct.unpack_from('<I', dat, damage_column + iid * 4)[0]
        if not gfi:
            continue
        path = cs.grp_path(gfi)
        lo = read(path)
        first = len(anchors)
        anchors.extend(parse_damage_lo(lo))
        if len(anchors) > first:
            actors.append((raw, first, len(anchors) - first))
        lo_hashes[path] = hashlib.sha256(lo).hexdigest()
    lines.append(f'static const Sc64BuildingFireAnchor sc64_building_fire_anchors[{len(anchors)}] = {{')
    lines.extend(f'    {{ {x}, {y}, {slot} }},' for x, y, slot in anchors)
    lines.append('};')
    lines.append(f'static const Sc64BuildingFireActor sc64_building_fire_actors[{len(actors)}] = {{')
    lines.extend(f'    {{ {raw}, {first}, {count} }},' for raw, first, count in actors)
    lines.extend(['};', f'#define SC64_BUILDING_FIRE_ACTOR_COUNT {len(actors)}u'])
    lines.append('#endif')
    header_path.parent.mkdir(parents=True, exist_ok=True)
    header_path.write_text('\n'.join(lines) + '\n', encoding='ascii')
    manifest = {'rom_sha1': sha, 'reference': 'OpenBW 4b046d5f65302b10cb0a745f0fecd37ec85b20a8 bwenums.h and bwgame.h update_unit_damage_overlay', 'timing': 'flames: 12-frame loop; bleeding: 6-frame loop; 2 source updates per frame', 'palette': 'opaque UI ramp approximates draw function 9', 'anchors': 'frame-zero authenticated source damage LO offsets; deterministic slot order', 'lo_sha256': lo_hashes, 'actor_count': len(actors), 'images': [{k: im[k] for k in ('id', 'grp', 'sha1')} for im in images], 'pixel_bytes': sum(len(f['pixels']) for im in images for f in im['frames'])}
    out_dir.mkdir(parents=True, exist_ok=True)
    (out_dir / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    return manifest

def main(argv: Sequence[str] | None = None) -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--rom', type=Path, default=cs.ROM_DEFAULT)
    p.add_argument('--palette-remap', type=Path, default=cs.REMAP_DEFAULT)
    p.add_argument('--header', type=Path, default=ROOT / '.local/sc64/building-fire/generated/sc64_building_fire_generated.h')
    p.add_argument('--out', type=Path, default=ROOT / '.local/sc64/building-fire/generated')
    p.add_argument('--upstream', type=Path, default=cs.UPSTREAM)
    args = p.parse_args(argv)
    try:
        m = generate(args.rom, args.palette_remap, args.header, args.out, args.upstream)
    except (OSError, ValueError, KeyError, RuntimeError) as e:
        print(f'sc64_building_fire_pack: {e}', file=sys.stderr)
        return 1
    print(f"Imported {len(m['images'])} fire banks, {m['pixel_bytes']} pixel bytes")
    return 0
if __name__ == '__main__':
    raise SystemExit(main())
