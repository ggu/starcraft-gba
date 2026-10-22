#!/usr/bin/env python3
"""Read compiled campaign availability from a GBA ROM and its matching ELF.

The catalog's records are decoded using their ELF symbol address and DWARF
layout, so experimental mission gates are reported from the actual build.
This tool only reads files; it does not launch an emulator.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from emulator_smoke import symbol_sizes, symbols  # noqa: E402
from sc64_races_profile import dwarf_struct  # noqa: E402

ROM_BASE = 0x08000000
CATALOG_SYMBOL = 'sc64_campaign_catalog'
FIELDS = ('id', 'faction', 'chapter', 'playable')
PROFILE_FACTIONS = {'terran': 0, 'zerg': 1, 'protoss': 2}


def parse_catalog_blob(data: bytes, stride: int, offsets: dict[str, int],
                       profile: str) -> dict:
    """Decode and validate catalog records from their raw target bytes."""
    if profile not in (*PROFILE_FACTIONS, 'unified'):
        raise ValueError(f'Unknown campaign profile: {profile}')
    if stride <= 0 or len(data) == 0 or len(data) % stride:
        raise ValueError('Catalog bytes must contain whole, nonempty records')
    if any(name not in offsets for name in FIELDS):
        raise ValueError('DWARF catalog layout is missing an integer field')
    if any(offset < 0 or offset + 4 > stride for offset in (offsets[n] for n in FIELDS)):
        raise ValueError('DWARF catalog field extends beyond its record')

    records = []
    seen_ids: set[int] = set()
    count = len(data) // stride
    for index in range(count):
        row = data[index * stride:(index + 1) * stride]
        values = {name: struct.unpack_from('<i', row, offsets[name])[0] for name in FIELDS}
        if values['id'] < 0 or values['id'] in seen_ids:
            raise ValueError(f'Invalid or duplicate mission id {values["id"]}')
        if values['faction'] not in (0, 1, 2):
            raise ValueError(f'Invalid faction for mission {values["id"]}')
        if not 1 <= values['chapter'] <= 10:
            raise ValueError(f'Invalid chapter for mission {values["id"]}')
        if values['playable'] not in (0, 1):
            raise ValueError(f'Invalid playable flag for mission {values["id"]}')
        seen_ids.add(values['id'])
        if 'title' in offsets and offsets['title'] + 4 <= stride:
            values['title_address'] = struct.unpack_from('<I', row, offsets['title'])[0]
        records.append(values)

    faction = PROFILE_FACTIONS.get(profile)
    enabled = [row for row in records if row['playable'] and
               (faction is None or row['faction'] == faction)]
    enabled.sort(key=(lambda row: row['id']) if faction is None else
                 (lambda row: (row['chapter'], row['id'])))
    return {
        'profile': profile,
        'record_count': count,
        'enabled_mission_ids': [row['id'] for row in enabled],
        'enabled_missions': [{key: row[key] for key in ('id', 'faction', 'chapter')}
                             for row in enabled],
        'records': [{key: row[key] for key in ('id', 'faction', 'chapter', 'playable')}
                    for row in records],
    }


def read_campaign_metadata(rom_path: Path, elf_path: Path, profile: str) -> dict:
    """Read `sc64_campaign_catalog` using ELF symbols and DWARF offsets."""
    elf_symbols = symbols(elf_path)
    sizes = symbol_sizes(elf_path)
    if CATALOG_SYMBOL not in elf_symbols or CATALOG_SYMBOL not in sizes:
        raise RuntimeError(f'{elf_path.name} does not contain {CATALOG_SYMBOL}')
    stride, offsets = dwarf_struct(elf_path, 'Sc64MissionInfo')
    for field in FIELDS:
        if field not in offsets:
            raise RuntimeError(f'Sc64MissionInfo DWARF data is missing {field}')
    address = elf_symbols[CATALOG_SYMBOL]
    if address < ROM_BASE:
        raise RuntimeError(f'{CATALOG_SYMBOL} is outside the GBA ROM address range')
    size = sizes[CATALOG_SYMBOL]
    if size <= 0 or size % stride:
        raise RuntimeError(f'{CATALOG_SYMBOL} ELF size {size} is not a multiple of stride {stride}')
    blob_offset = address - ROM_BASE
    rom = rom_path.read_bytes()
    if blob_offset < 0 or blob_offset + size > len(rom):
        raise RuntimeError(f'{CATALOG_SYMBOL} at ROM offset {blob_offset:#x} exceeds ROM size {len(rom):#x}')
    result = parse_catalog_blob(rom[blob_offset:blob_offset + size], stride, offsets, profile)
    result.update({'elf': str(elf_path), 'rom': str(rom_path),
                   'symbol_address': f'0x{address:08x}', 'rom_offset': blob_offset,
                   'symbol_size': size, 'record_stride': stride})
    return result


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--elf', type=Path, required=True, help='ELF matching the ROM')
    parser.add_argument('--rom', type=Path, required=True, help='GBA ROM matching the ELF')
    parser.add_argument('--profile', choices=(*PROFILE_FACTIONS, 'unified'), required=True,
                        help='campaign profile to filter enabled mission IDs for')
    args = parser.parse_args(argv)
    try:
        result = read_campaign_metadata(args.rom, args.elf, args.profile)
    except (OSError, RuntimeError, ValueError) as exc:
        print(f'sc64_campaign_metadata: {exc}', file=sys.stderr)
        return 1
    print(json.dumps(result, indent=2))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
