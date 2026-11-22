#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Derive each placed Reaver's starting Scarab count from the source map data.

The shared importer (tools/sc64_import.py) drops the per-unit hangar count that a
StarCraft map UNIT record carries (offset 24, valid when bit 0x20 of the valid-fields
word is set). This tool reads the same authenticated source.chk files, read-only, and
writes a small private table that the paid-Scarab service looks up by mission, source
owner and position. Nothing is written beside the source data: the output goes under
build/generated/ (the default), never into the source-map folders.

Missions whose CHK is present are "covered": a Reaver of a covered mission that is not
in the table (reinforced, rescued, created by a trigger) is not a placed Reaver. Missions
without a CHK here (P1-P3, Z1-Z3 and the Terran missions) are not covered; their imported
unit tables place no Reaver.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_SOURCE_ROOT = ROOT / '.local/sc64/zp-stage/generated/source_maps'
DEFAULT_OUTPUT = ROOT / 'build/generated/placed-hangar/sc64_placed_hangar_generated.h'
UNIT_BYTES = 36
REAVER = 83
HANGAR_VALID = 0x20
# Mission name -> the global mission id the runtime keeps in race_selected_mission.
MISSION_IDS = {'Z4': 15, 'Z5': 16, 'Z6': 17, 'Z7': 18, 'Z8': 19, 'Z9': 20, 'Z10': 21,
               'P4': 23, 'P5': 24, 'P6': 25, 'P7': 26, 'P8': 27, 'P9': 28, 'P10': 29}


def sections(chk: bytes) -> dict[bytes, bytes]:
    out: dict[bytes, bytes] = {}
    pos = 0
    while pos < len(chk):
        if len(chk) - pos < 8:
            raise ValueError('source CHK has a truncated section header')
        tag = chk[pos:pos + 4]
        size = struct.unpack_from('<I', chk, pos + 4)[0]
        end = pos + 8 + size
        if end > len(chk):
            raise ValueError(f'source CHK section {tag!r} exceeds file bounds')
        if tag in out:
            raise ValueError(f'source CHK has duplicate section {tag!r}')
        out[tag] = chk[pos + 8:end]
        pos = end
    return out


def placed_reavers(unit: bytes) -> list[dict]:
    if len(unit) % UNIT_BYTES:
        raise ValueError('UNIT is not a whole number of 36-byte records')
    rows = []
    for index in range(len(unit) // UNIT_BYTES):
        raw = unit[index * UNIT_BYTES:(index + 1) * UNIT_BYTES]
        if struct.unpack_from('<H', raw, 8)[0] != REAVER:
            continue
        x, y = struct.unpack_from('<HH', raw, 4)
        valid = struct.unpack_from('<H', raw, 14)[0]
        hangar = struct.unpack_from('<H', raw, 24)[0]
        if not valid & HANGAR_VALID:
            raise ValueError(f'record {index}: a placed Reaver without a valid hangar field')
        if hangar > 5:
            raise ValueError(f'record {index}: hangar count {hangar} exceeds the capacity of 5')
        # Imported units keep the source position floor-divided by two (16 px world units).
        rows.append({'record': index, 'owner': raw[16], 'x': x // 2, 'y': y // 2, 'hangar': hangar})
    return rows


def read_missions(source_root: Path) -> dict[str, list[dict]]:
    result: dict[str, list[dict]] = {}
    for name in MISSION_IDS:
        folder = source_root / name.lower()
        chk_path = folder / 'source.chk'
        if not chk_path.is_file():
            continue
        chk = chk_path.read_bytes()
        manifest = json.loads((folder / 'manifest.json').read_text())
        if hashlib.sha256(chk).hexdigest() != manifest.get('source_chk_sha256'):
            raise ValueError(f'{name} CHK does not match its manifest')
        result[name] = placed_reavers(sections(chk).get(b'UNIT', b''))
    if not result:
        raise ValueError('no source CHK found')
    return result


def emit(missions: dict[str, list[dict]]) -> str:
    mask = 0
    rows = []
    for name in sorted(missions, key=MISSION_IDS.get):
        mask |= 1 << MISSION_IDS[name]
        for row in missions[name]:
            rows.append((MISSION_IDS[name], row))
    lines = ['/* Generated privately from authenticated source CHKs; do not edit. */',
             '#ifndef SC64_PLACED_HANGAR_GENERATED_H',
             '#define SC64_PLACED_HANGAR_GENERATED_H',
             f'#define SC64_PLACED_HANGAR_COVERED_MASK 0x{mask:08x}u',
             f'#define SC64_PLACED_HANGAR_COUNT {len(rows)}',
             'typedef struct { unsigned char mission, owner, hangar; unsigned short x_px, y_px; } Sc64PlacedHangar;',
             f'static const Sc64PlacedHangar sc64_placed_hangar[{max(len(rows), 1)}] = {{']
    for mission, row in rows:
        lines.append(f"    {{{mission}u,{row['owner']}u,{row['hangar']}u,{row['x']}u,{row['y']}u}},")
    if not rows:
        lines.append('    {0u,0u,0u,0u,0u},')
    lines += ['};', '#endif', '']
    return '\n'.join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-root', type=Path, default=DEFAULT_SOURCE_ROOT)
    parser.add_argument('--output', type=Path, default=DEFAULT_OUTPUT)
    args = parser.parse_args()
    missions = read_missions(args.source_root)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(emit(missions))
    total = sum(len(rows) for rows in missions.values())
    print(f'placed Reaver hangar table: {total} Reavers in {len(missions)} missions -> {args.output}')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
