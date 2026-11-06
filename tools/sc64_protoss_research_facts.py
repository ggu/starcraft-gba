#!/usr/bin/env python3
"""Read authenticated N64 upgrade facts without writing private data.

SPDX-License-Identifier: GPL-3.0-or-later
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from tools.sc64_sprites import load_extractor
from tools.sc64_construction_sprites import EXPECTED_ROM_SHA1

EXTRACTOR_SHA256 = '6a84761a9a8074fcc393527a7b17cea194084e8ab49176ff82fa3bef8e1b1dc2'
TABLE_SHA256 = '5c7a104f37eab018d15719066052c2db25ef046d9330770045d7e4977f98c12b'
UNIT_SHA256 = '54356d7585c66cd463346996444fd208336eaacf6335d30eb2688aa73c9cb2df'
WEAPON_SHA256 = '764c6c1535a8ba5d3097c62c5755769ab9d1217e9ef4621e64720a8d2a22e730'
COUNT = 61
UNIT_COUNT, WEAPON_COUNT = 228, 130
ASSOCIATION_UNITS = (64, 65, 66, 68, 70, 71, 72, 73, 83, 85)
ROWS = {5: 'ground_armor', 6: 'air_armor', 13: 'ground_weapons',
        14: 'air_weapons', 15: 'plasma_shields', 33: 'singularity_charge'}
FIELDS = ('minerals_base', 'minerals_factor', 'gas_base', 'gas_factor',
          'time_base', 'time_factor')

def decode(data):
    """Decode only authenticated, fixed-length N64 upgrades.dat fields."""
    if len(data) != COUNT * 21 or hashlib.sha256(data).hexdigest() != TABLE_SHA256:
        raise ValueError('upgrades.dat authentication failed')
    result = {}
    for ident, name in ROWS.items():
        row = {field: struct.unpack_from('<H', data, 2 * (COUNT * col + ident))[0]
               for col, field in enumerate(FIELDS)}
        row['max_level'] = data[COUNT * 19 + ident]
        row['source_id'] = ident
        if row['max_level'] not in (1, 3) or not row['time_base']:
            raise ValueError('invalid upgrade row')
        result[name] = row
    return result

def decode_associations(units, weapons):
    """Selected raw-unit armor and weapon upgrade identities, not damage."""
    if (len(units) != 19876 or hashlib.sha256(units).hexdigest() != UNIT_SHA256 or
            len(weapons) != 5460 or hashlib.sha256(weapons).hexdigest() != WEAPON_SHA256):
        raise ValueError('unit/weapon table authentication failed')
    result = {}
    for ident in ASSOCIATION_UNITS:
        ground, air = units[5892 + ident], units[6348 + ident]
        if ground > WEAPON_COUNT or air > WEAPON_COUNT:
            raise ValueError('invalid weapon identity')
        result[str(ident)] = {
            'armor_upgrade': units[8400 + ident],
            'ground_weapon': ground, 'air_weapon': air,
            'ground_upgrade': weapons[2210 + ground] if ground < WEAPON_COUNT else None,
            'air_upgrade': weapons[2210 + air] if air < WEAPON_COUNT else None,
        }
    return result

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--rom', type=Path, required=True)
    p.add_argument('--extractor', type=Path, required=True)
    a = p.parse_args()
    if hashlib.sha256(a.extractor.read_bytes()).hexdigest() != EXTRACTOR_SHA256:
        p.error('extractor authentication failed')
    extractor = load_extractor(a.extractor)
    rom = extractor.load_rom(str(a.rom))
    if hashlib.sha1(rom).hexdigest() != EXPECTED_ROM_SHA1:
        p.error('ROM authentication failed')
    archive = extractor.BoltArchive(rom)
    entries = {}
    for entry in archive.entries():
        if entry.path in ('00C/001', '00C/006', '00C/007'):
            if entry.path in entries: p.error('source table duplicated')
            entries[entry.path] = entry
    if len(entries) != 3: p.error('source table missing')
    upgrades = archive.read(entries['00C/006'])
    units, weapons = archive.read(entries['00C/001']), archive.read(entries['00C/007'])
    print(json.dumps({'rom_sha1': EXPECTED_ROM_SHA1, 'table_sha256': TABLE_SHA256,
                      'rows': decode(upgrades), 'associations': decode_associations(units, weapons)},
                     indent=2, sort_keys=True))

if __name__ == '__main__':
    main()
