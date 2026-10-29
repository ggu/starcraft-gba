#!/usr/bin/env python3
"""Derive private Shield Battery properties from an authenticated source CHK.

P7 is the default. Later Protoss missions that run the shared unit services
(currently P8) use --mission. Map coordinates and starting properties belong
in the generated local header, not in the source repository. Authentication
uses the shared scenario packer.
"""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_SOURCE_ROOT = ROOT / '.local/sc64/zp-stage/generated/source_maps'
DEFAULT_OUTPUT = ROOT / '.local/sc64/zp-stage/generated/scenarios/sc64_p7_battery_properties_generated.h'
UNIT_BYTES = 36
BATTERY_RAW_TYPE = 172
# Expected source-owner roster per mission; the runtime binds every row.
MISSION_OWNERS = {'P7': [0, 4], 'P8': [3, 3]}


def _scenario_packer():
    path = ROOT / 'tools/sc64_zp_scenario_pack.py'
    spec = importlib.util.spec_from_file_location('sc64_p7_battery_scenario', path)
    if spec is None or spec.loader is None:
        raise RuntimeError('cannot load the source scenario authenticator')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def decode_batteries(payload: bytes, width: int, height: int) -> list[dict]:
    """Read UNIT placement/property fields, without supplying map defaults."""
    if not payload or len(payload) % UNIT_BYTES:
        raise ValueError('UNIT must contain complete 36-byte records')
    if not 1 <= width <= 128 or not 1 <= height <= 128:
        raise ValueError('map dimensions are outside the imported-map bounds')
    rows = []
    for offset in range(0, len(payload), UNIT_BYTES):
        x, y, raw = struct.unpack_from('<HHH', payload, offset + 4)
        if raw != BATTERY_RAW_TYPE:
            continue
        valid = struct.unpack_from('<H', payload, offset + 14)[0]
        owner, hp, shields, energy = payload[offset + 16:offset + 20]
        if owner >= 8 or x >= width * 32 or y >= height * 32:
            raise ValueError('Battery owner or placement is out of bounds')
        if valid != 0x0f or any(value > 100 for value in (hp, shields, energy)):
            raise ValueError('Battery starting properties are invalid or incomplete')
        rows.append({'row': offset // UNIT_BYTES, 'x_source': x, 'y_source': y,
                     'valid': valid, 'owner': owner, 'hp': hp,
                     'shields': shields, 'energy': energy})
    return rows


def parse_source(source_root: Path, mission: str = 'P7') -> dict:
    mission = mission.upper()
    if mission not in MISSION_OWNERS:
        raise ValueError('mission must be one of ' + ', '.join(MISSION_OWNERS))
    decoder = _scenario_packer()
    scenario = decoder.parse_source(source_root, mission)
    chk = (source_root / mission.lower() / 'source.chk').read_bytes()
    digest = hashlib.sha256(chk).hexdigest()
    if digest != scenario['source_chk_sha256']:
        raise ValueError(f'{mission} CHK changed after scenario authentication')
    sections = decoder.parse_chk_sections(chk)
    rows = decode_batteries(sections.get(b'UNIT', b''),
                           scenario['width'], scenario['height'])
    # The runtime keeps the original enemy source owners distinct.
    rows.sort(key=lambda row: (row['owner'], row['row']))
    if [row['owner'] for row in rows] != MISSION_OWNERS[mission]:
        raise ValueError(f'{mission} Battery source-owner roster differs from the runtime')
    return {'mission': mission, 'source_chk_sha256': digest, 'rows': rows}


def emit_header(data: dict) -> str:
    p = data.get('mission', 'P7')
    lower = p.lower()
    lines = [f'/* Generated privately from authenticated {p} source.chk; do not edit. */',
             f'#ifndef SC64_{p}_BATTERY_PROPERTIES_GENERATED_H',
             f'#define SC64_{p}_BATTERY_PROPERTIES_GENERATED_H',
             f'#define SC64_{p}_BATTERY_SOURCE_CHK_SHA256 "' + data['source_chk_sha256'] + '"']
    if p == 'P7':
        lines.append('typedef struct { unsigned short row, x_source, y_source, valid; unsigned char owner, hp, shields, energy; } Sc64P7BatterySourceProperty;')
    lines.append(f'static const Sc64P7BatterySourceProperty sc64_{lower}_battery_source_properties[{len(data["rows"])}] = {{')
    for row in data['rows']:
        values = (row[key] for key in ('row', 'x_source', 'y_source', 'valid',
                                       'owner', 'hp', 'shields', 'energy'))
        lines.append('    {' + ','.join(f'{value}u' for value in values) + '},')
    lines.extend(['};', '#endif', ''])
    return '\n'.join(lines)


def generate(source_root: Path, output: Path, mission: str = 'P7') -> dict:
    data = parse_source(source_root, mission)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(emit_header(data), encoding='utf-8')
    return {'output': str(output), 'source_chk_sha256': data['source_chk_sha256'],
            'battery_count': len(data['rows'])}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-root', type=Path, default=DEFAULT_SOURCE_ROOT)
    parser.add_argument('--output', type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument('--mission', default='P7', choices=sorted(MISSION_OWNERS))
    args = parser.parse_args()
    if args.mission != 'P7' and args.output == DEFAULT_OUTPUT:
        parser.error('a later mission needs its own --output')
    try:
        result = generate(args.source_root, args.output, args.mission)
    except (OSError, ValueError, RuntimeError) as exc:
        parser.error(str(exc))
    print(json.dumps(result, sort_keys=True))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
