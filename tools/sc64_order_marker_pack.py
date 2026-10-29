#!/usr/bin/env python3
"""Extract the ROM's cursor-marker candidate into a private GBA header.

Authenticated N64 tables link sprite 318 to image 445, and its IScript
GndAttkInit plays GRP frames 0..4 before hiding the cursor marker. The pinned
OpenBW enum calls these Cursor_Marker; its use for PC destination feedback is
reference evidence, not a trace of the N64 command UI or its attack mapping.

Copyright (C) 2026 blizzard-gba contributors
SPDX-License-Identifier: GPL-3.0-or-later
"""
from __future__ import annotations
import argparse
import hashlib
import json
import sys
from pathlib import Path
from typing import Sequence

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from tools import sc64_construction_sprites as cs
from tools import sc64_death_sprites as death
from tools import sc64_sprites as base

SPRITE_ID = 318
IMAGE_ID = 445
GRP_PATH = '00E/0CE'

def compile_timeline(payload: bytes, start: int) -> list[tuple[int, int]]:
    """Accept only frame/wait pairs ending in hidecursormarker."""
    pc = start
    frame = None
    steps = []
    for _ in range(32):
        opcode, args, pc, _ = cs.decode_instruction(payload, pc)
        if opcode == 0 and frame is None:
            frame = args[0]
        elif opcode == 5 and frame is not None and args[0] > 0:
            steps.append((frame, args[0]))
            frame = None
        elif opcode == 45 and frame is None and steps:
            return steps
        else:
            raise ValueError(f'unsupported marker IScript opcode {opcode}')
    raise ValueError('marker IScript exceeds instruction bound')

def generate(rom_path: Path, remap_path: Path, header_path: Path, out_dir: Path,
             upstream: Path) -> dict:
    extractor = base.load_extractor(upstream)
    rom = extractor.load_rom(str(rom_path))
    rom_sha = hashlib.sha1(rom).hexdigest()
    if rom_sha != cs.EXPECTED_ROM_SHA1:
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
    _, _, _, sprite_images, grps, image_scripts = cs.table_images(
        tables['00C/001'], tables['00C/002'], tables['00C/003'], tables['00C/000'])
    dat = tables['00C/000']
    if (sprite_images[SPRITE_ID] != IMAGE_ID or cs.grp_path(grps[IMAGE_ID]) != GRP_PATH or
        dat[cs.IMAGE_COUNT * 8 + IMAGE_ID] != 0 or dat[cs.IMAGE_COUNT * 9 + IMAGE_ID] != 0):
        raise ValueError('cursor marker image association changed')
    payload = tables['012/002']
    script_id = image_scripts[IMAGE_ID]
    scripts = cs.script_index(payload)
    starts = cs.image_animation_starts(payload, scripts[script_id])
    steps = compile_timeline(payload, starts['GndAttkInit'])
    if steps != [(i, 1) for i in range(5)]:
        raise ValueError('unexpected marker timeline')
    data = read(GRP_PATH)
    grp = base.parse_grp(data)
    if grp['width'] != 22 or grp['height'] != 14 or len(grp['frames']) != 5:
        raise ValueError('unexpected marker frame dimensions')
    remap = remap_path.read_bytes()
    if len(remap) != 256:
        raise ValueError('palette remap must contain 256 bytes')
    frames = death.convert_frames(grp, range(5), 0, 0, remap, [])
    lines = ['/* Private generated source art. Do not commit. */', '#ifndef SC64_ORDER_MARKER_GENERATED_H', '#define SC64_ORDER_MARKER_GENERATED_H', '#define SC64_ORDER_MARKER_BANK_VERSION 1u']
    for i, f in enumerate(frames):
        if not (0 < f['w'] <= 22 and 0 < f['h'] <= 14):
            raise ValueError('marker frame exceeds dimensions')
        lines.append(f'static const unsigned char sc64_order_marker_pixels_{i}[] = {{')
        for off in range(0, len(f['pixels']), 16):
            lines.append('    ' + ', '.join(str(v) for v in f['pixels'][off:off+16]) + ',')
        lines.append('};')
    lines.append('static const Sc64OrderMarkerFrame sc64_order_marker_frames[5] = {')
    for i, f in enumerate(frames):
        lines.append(f"    {{ {f['x']}, {f['y']}, {f['w']}, {f['h']}, sc64_order_marker_pixels_{i} }},")
    lines.extend(['};', '#endif'])
    header_path.parent.mkdir(parents=True, exist_ok=True)
    header_path.write_text('\n'.join(lines)+'\n', encoding='ascii')
    manifest = {'rom_sha1': rom_sha, 'sprite_id': SPRITE_ID, 'image_id': IMAGE_ID,
                'grp_path': GRP_PATH, 'grp_sha1': hashlib.sha1(data).hexdigest(),
                'iscript_id': script_id, 'timeline': steps, 'canvas': [22,14],
                'palette_remap_sha256': hashlib.sha256(remap).hexdigest(),
                'identity': 'PC-reference cursor marker; N64 input behavior not traced',
                'limitations': 'No separate source red/attack marker identified; frame placement uses canvas center'}
    out_dir.mkdir(parents=True, exist_ok=True)
    (out_dir/'manifest.json').write_text(json.dumps(manifest, indent=2)+'\n')
    return manifest

def main(argv: Sequence[str] | None = None) -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--rom', type=Path, default=cs.ROM_DEFAULT)
    p.add_argument('--palette-remap', type=Path, default=cs.REMAP_DEFAULT)
    p.add_argument('--header', type=Path, default=ROOT/'.local/sc64/order-marker/generated/sc64_order_marker_generated.h')
    p.add_argument('--out', type=Path, default=ROOT/'.local/sc64/order-marker/generated')
    p.add_argument('--upstream', type=Path, default=cs.UPSTREAM)
    args=p.parse_args(argv)
    try:
        manifest=generate(args.rom,args.palette_remap,args.header,args.out,args.upstream)
    except (OSError, ValueError, KeyError, RuntimeError) as exc:
        print(f'sc64_order_marker_pack: {exc}',file=sys.stderr)
        return 1
    print(f"Imported cursor marker: {len(manifest['timeline'])} source frames")
    return 0
if __name__ == '__main__':
    raise SystemExit(main())
