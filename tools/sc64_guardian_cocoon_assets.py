#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Prepare raw Cocoon source frames privately; this does not change a ROM bank."""
from __future__ import annotations
import argparse
import hashlib
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from tools import sc64_sprites as sprites
from tools import sc64_construction_sprites as construction
from tools import sc64_unit_attack_sprites as attacks

EXTRACTOR_SHA256 = '6a84761a9a8074fcc393527a7b17cea194084e8ab49176ff82fa3bef8e1b1dc2'
COCOON_GRP_SHA256 = '3257f8e06e504a6b2c4ea7f406de9375d15b183ca993dc994807ca82ade05a0d'

def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()

def prepare(rom: Path, extractor_path: Path, out: Path) -> dict:
    if not out.resolve().is_relative_to((ROOT / '.local').resolve()):
        raise ValueError('private output must be under this worktree .local')
    extractor_bytes = extractor_path.read_bytes()
    if digest(extractor_bytes) != EXTRACTOR_SHA256:
        raise ValueError('extractor does not match the pinned source')
    extractor = sprites.load_extractor(extractor_path)
    normalized = extractor.load_rom(str(rom))
    if hashlib.sha1(normalized).hexdigest() != sprites.EXPECTED_USA_ROM_SHA1:
        raise ValueError('ROM does not match the verified USA source')
    archive = extractor.BoltArchive(normalized)
    ref = sprites.resolve_grp_indices(archive, {'cocoon': 59})['cocoon']
    grp_data = sprites._entry_payload(archive, ref['bolt_path'])
    if digest(grp_data) != COCOON_GRP_SHA256:
        raise ValueError('Cocoon GRP does not match the source pin')
    grp = sprites.parse_grp(grp_data)
    if (grp['width'], grp['height'], len(grp['frames'])) != (48, 48, 23):
        raise ValueError('unexpected Cocoon GRP geometry')
    images = sprites._entry_payload(archive, '00C/000')
    direction, scripts, grp_ids = attacks.image_tables(images)
    payload = sprites._entry_payload(archive, '012/002')
    index = construction.script_index(payload)
    if ref['image_id'] != 11 or scripts[11] != 8 or scripts[12] != 275 or grp_ids[11] != grp_ids[12] or direction[11]:
        raise ValueError('unexpected source image mapping')
    animations = {}
    for image in (11, 12):
        starts = construction.image_animation_starts(payload, index[scripts[image]])
        animations[str(image)] = {name: construction.animation_instructions(payload, start)
                                  for name, start in starts.items() if start}
    if (animations.get('11', {}).get('Init', [None])[0] != (9, [12, 0, 42]) or
            not any(opcode == 29 for opcode, _ in animations.get('12', {}).get('Init', []))):
        raise ValueError('unexpected Cocoon underlay relationship')
    indices = b''.join(bytes(frame['pixels']) for frame in grp['frames'])
    alpha = b''.join(bytes(frame['alpha']) for frame in grp['frames'])
    palette = sprites._entry_payload(archive, '00B/004')
    report = {'unit': ref, 'rom_sha1': sprites.EXPECTED_USA_ROM_SHA1,
              'extractor_sha256': EXTRACTOR_SHA256, 'grp_sha256': COCOON_GRP_SHA256,
              'frame_count': 23, 'width': 48, 'height': 48,
              'indexed_sha256': digest(indices), 'alpha_sha256': digest(alpha),
              'palette_sha256': digest(palette), 'images': animations,
              'child_underlay': {'image': 12, 'x': 0, 'y': 42,
                                 'follow_main_graphic': True},
              'timing_scope': 'decoded instructions only; native timing unverified',
              'render_scope': 'source frames only; underlay composition unimplemented'}
    out.mkdir(parents=True, exist_ok=True)
    (out / 'frames-indexed.bin').write_bytes(indices)
    (out / 'frames-alpha.bin').write_bytes(alpha)
    (out / 'palette.bin').write_bytes(palette)
    (out / 'manifest.json').write_text(json.dumps(report, indent=2) + '\n')
    return report

def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rom', required=True, type=Path)
    parser.add_argument('--extractor', required=True, type=Path)
    parser.add_argument('--out', required=True, type=Path)
    args = parser.parse_args()
    try:
        report = prepare(args.rom, args.extractor, args.out)
    except (OSError, ValueError, KeyError) as exc:
        print(f'Cocoon asset preparation: {exc}', file=sys.stderr)
        return 1
    print(f"Prepared {report['frame_count']} private source frames")
    return 0

if __name__ == '__main__':
    raise SystemExit(main())
