#!/usr/bin/env python3
"""Generate private per-direction unit frames from a verified SC64 ROM.

StarCraft unit GRPs store each animation step as 17 frames: facing north,
then clockwise to south. The other 15 directions are the mirrored frames. The
race sprite importer keeps only frame 0, so units always face north. This
tool writes frames 0-16 of every imported unit GRP (plus the Siege Tank
turret and siege-mode subunits) to a separate header for the default-off
SC64_UNIT_FACING presentation path. Output stays in ignored .local/ folders.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from tools import sc64_race_sprites as races  # noqa: E402
from tools import sc64_sprites as base  # noqa: E402

OUT = ROOT / '.local/sc64/facing/generated'
DIRECTIONS = 17
# Brood War raw ids below 106 are units; higher ids are buildings and props.
FIRST_BUILDING_ID = 106
# Subunits drawn with the Siege Tank: tank-mode turret, siege-mode hull and turret.
EXTRA_UNIT_IDS = {'siege_tank_turret': 6, 'siege_tank_siege': 30, 'siege_tank_siege_turret': 31}


def unit_ids() -> dict[str, int]:
    merged: dict[str, int] = {}
    seen: set[int] = set()
    tables = [races.RACE_UNIT_IDS, races.T4_UNIT_IDS, races.T6_UNIT_IDS, races.LATE_UNIT_IDS,
              races.ZP_UNIT_IDS, races.Z5_UNIT_IDS, races.Z6_UNIT_IDS, races.P6_UNIT_IDS,
              races.P7_UNIT_IDS, EXTRA_UNIT_IDS]
    for table in tables:
        for name, raw in table.items():
            if raw < FIRST_BUILDING_ID and raw not in seen:
                merged[name] = raw
                seen.add(raw)
    return merged


def frame_pixels(frame: dict, remap: bytes) -> bytes:
    data = bytearray(base.crop_frame(frame))
    x, y, fw = frame['x'], frame['y'], frame['frame_width']
    for row in range(frame['frame_height']):
        for col in range(fw):
            si = (y + row) * frame['width'] + x + col
            di = row * fw + col
            data[di] = remap[data[di]] if frame['alpha'][si] else 0
    return bytes(data)


def generate(rom: Path, remap_path: Path, out: Path) -> dict:
    extractor = base.load_extractor(base.UPSTREAM)
    normalized = extractor.load_rom(str(rom))
    sha1 = hashlib.sha1(normalized).hexdigest()
    if sha1 != base.EXPECTED_USA_ROM_SHA1:
        raise ValueError(f'Expected verified USA ROM {base.EXPECTED_USA_ROM_SHA1}, got {sha1}')
    remap = remap_path.read_bytes()
    if len(remap) != 256:
        raise ValueError('sprite remap must have 256 entries')
    archive = extractor.BoltArchive(normalized)
    entries = {entry.path: entry for entry in archive.entries()}
    refs = base.resolve_grp_indices(archive, unit_ids())
    lines = ['/* Generated from the verified USA ROM; private local asset. */',
             '#ifndef SC64_UNIT_FACING_GENERATED_H',
             '#define SC64_UNIT_FACING_GENERATED_H',
             '#ifndef SC64_SPRITES_GENERATED_H',
             '#ifndef SC64_RACE_SPRITES_GENERATED_H',
             'typedef struct { short offset_x, offset_y; unsigned short w, h, canvas_w, canvas_h; '
             'const unsigned char *pixels; } Sc64SpriteFrame;',
             '#endif', '#endif']
    manifest, cases, total = [], [], 0
    for name, ref in sorted(refs.items(), key=lambda item: item[1]['unit_id']):
        grp_data = archive.read(entries[ref['bolt_path']])
        grp = base.parse_grp(grp_data)
        static = len(grp['frames']) < DIRECTIONS
        if static and name not in EXTRA_UNIT_IDS:
            manifest.append({'name': name, 'unit_id': ref['unit_id'], 'frames': len(grp['frames']),
                             'skipped': 'fewer than 17 frames (non-directional)'})
            continue
        symbol = f'sc64_unit_facing_{ref["unit_id"]}'
        frames = []
        for d in range(1 if static else DIRECTIONS):
            frame = grp['frames'][d]
            data = frame_pixels(frame, remap)
            total += len(data)
            lines.append(f'static const unsigned char {symbol}_{d}_pixels[{len(data)}] = {{')
            for i in range(0, len(data), 16):
                lines.append('    ' + ','.join(f'0x{v:02x}' for v in data[i:i + 16]) + ',')
            lines.append('};')
            frames.append(f'{{{frame["x"]},{frame["y"]},{frame["frame_width"]},{frame["frame_height"]},'
                          f'{frame["width"]},{frame["height"]},{symbol}_{d}_pixels}}')
        if static:  # e.g. the siege-mode hull, which does not rotate
            frames = frames * DIRECTIONS
        lines.append(f'static const Sc64SpriteFrame {symbol}[{DIRECTIONS}] = {{')
        lines.extend(f'    {item},' for item in frames)
        lines.append('};')
        cases.append(f'    case {ref["unit_id"]}: return {symbol};')
        manifest.append({'name': name, 'unit_id': ref['unit_id'], 'frames': len(grp['frames']),
                         'sha1': hashlib.sha1(grp_data).hexdigest()})
    lines += ['/* The 17 north-to-south frames for a raw unit id, or 0. */',
              'static inline const Sc64SpriteFrame *sc64_unit_facing_frames(unsigned short raw_type) {',
              '    switch (raw_type) {', *cases, '    default: return 0;', '    }', '}',
              f'#define SC64_UNIT_FACING_PIXEL_BYTES {total}u', '#endif', '']
    out.mkdir(parents=True, exist_ok=True)
    (out / 'sc64_unit_facing_generated.h').write_text('\n'.join(lines))
    summary = {'rom_sha1': sha1, 'units': len(cases), 'pixel_bytes': total, 'entries': manifest}
    (out / 'manifest.json').write_text(json.dumps(summary, indent=2) + '\n')
    return summary


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('--rom', type=Path, default=races.ROM)
    parser.add_argument('--remap', type=Path, default=races.PALETTE_REMAP)
    parser.add_argument('--out', type=Path, default=OUT)
    args = parser.parse_args()
    summary = generate(args.rom, args.remap, args.out)
    print(f'{summary["units"]} directional units, {summary["pixel_bytes"]} pixel bytes')
    for entry in summary['entries']:
        if 'skipped' in entry:
            print(f'  skipped {entry["name"]} raw{entry["unit_id"]}: {entry["skipped"]}')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
