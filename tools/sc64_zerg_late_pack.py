#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Authenticate and privately inventory the Z9 and Z10 source scenarios.

This mirrors ``sc64_z8_pack.py`` for the two final Zerg missions. It never
invokes the importer or regenerates shared banks. It consumes a previously
retained CHK, authenticates its manifest and ROM against a pinned per-mission
contract, then writes a complete numeric descriptor only to an explicit
ignored ``.local/build/z9-*`` or ``.local/build/z10-*`` path.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import sys
from typing import Any

TOOLS = Path(__file__).resolve().parent
if str(TOOLS) not in sys.path:
    sys.path.insert(0, str(TOOLS))

import sc64_z8_pack as z8  # noqa: E402  (shared authenticated row helpers)

ROOT = z8.ROOT
DEFAULT_SOURCE_ROOT = z8.DEFAULT_SOURCE_ROOT
DEFAULT_ROM = z8.DEFAULT_ROM
SCHEMA = 1

# Pinned per-mission contracts. Every number below was read from the
# authenticated USA ROM through the pinned extractor; a mismatch fails closed.
CONTRACTS: dict[str, dict[str, Any]] = {
    'Z9': {
        'global_id': 20,
        'map_path': '008/01C',
        'chk_sha256': '0eff2a2e44c55c0f5de9aa0c9123121f16cb870e1d326ae3a0fb3dba6bd8e5f7',
        'descriptor_sha256': '38921cb2bc5e10bc722e722952a2669eb99fb6096c8ab9684552aadf093bf123',
        'dimensions': [128, 128],
        'counts': {
            'trigger_records': 14, 'actions': 54, 'conditions': 16,
            'locations': 64, 'unit_records': 377, 'preserve_actions': 0,
            'create_actions': 2,
        },
        'action_histogram': {1: 1, 2: 1, 4: 4, 7: 3, 8: 4, 9: 3, 10: 3, 11: 2,
                             12: 3, 13: 5, 14: 1, 16: 18, 25: 1, 26: 4, 38: 1},
        'condition_histogram': {1: 1, 2: 4, 3: 6, 11: 4, 12: 1},
        'unit_histogram': {
            37: 6, 38: 2, 41: 4, 42: 3, 64: 18, 65: 48, 66: 16, 67: 7, 68: 4,
            69: 4, 70: 6, 71: 1, 72: 2, 83: 9, 84: 5, 131: 1, 139: 1, 142: 1,
            146: 1, 154: 5, 155: 5, 156: 69, 157: 3, 159: 2, 160: 6, 162: 30,
            163: 3, 164: 3, 165: 2, 166: 3, 167: 6, 169: 2, 170: 2, 171: 2,
            173: 1, 176: 19, 177: 32, 178: 30, 188: 5, 194: 2, 214: 6,
        },
        'ai_fourccs': ['VluA', 'Z9lo', 'Z9ly', 'Z9ro', 'Z9ry', 'Zer9'],
    },
    'Z10': {
        'global_id': 21,
        'map_path': '008/01D',
        'chk_sha256': '1a8f0e1de2ea9353e9561fb4b31da951852b32292f5963be416dc026c2862e17',
        'descriptor_sha256': '97c32263ea6b28b9d0f7d8916bf13f57ecb09788dead370741dea234d3b45cbf',
        'dimensions': [96, 128],
        'counts': {
            'trigger_records': 10, 'actions': 30, 'conditions': 10,
            'locations': 64, 'unit_records': 341, 'preserve_actions': 0,
            'create_actions': 2,
        },
        'action_histogram': {1: 1, 2: 1, 4: 5, 7: 4, 8: 1, 9: 1, 10: 2, 11: 2,
                             12: 2, 13: 2, 16: 5, 26: 4},
        'condition_histogram': {2: 5, 3: 1, 5: 1, 11: 1, 12: 1, 15: 1},
        'unit_histogram': {
            37: 6, 38: 3, 39: 1, 41: 3, 42: 4, 64: 14, 65: 30, 66: 13, 67: 1,
            68: 4, 69: 7, 70: 7, 71: 1, 72: 2, 83: 1, 84: 2, 131: 1, 143: 2,
            154: 5, 155: 3, 156: 54, 157: 6, 159: 3, 160: 6, 162: 34, 163: 3,
            164: 3, 165: 3, 166: 2, 167: 3, 169: 3, 170: 2, 171: 4, 172: 5,
            174: 1, 176: 17, 177: 34, 178: 41, 188: 2, 196: 1, 214: 4,
        },
        'ai_fourccs': ['Z10a', 'Z10b', 'Z10c', 'Z10d', 'Z10e'],
    },
}


def contract(mission: str) -> dict[str, Any]:
    try:
        return CONTRACTS[mission]
    except KeyError as exc:
        raise ValueError(f'unsupported late Zerg mission {mission!r}') from exc


def build_descriptor(mission: str, source_root: Path = DEFAULT_SOURCE_ROOT,
                     rom_path: Path = DEFAULT_ROM) -> dict[str, Any]:
    """Build the canonical private JSON descriptor from an already staged CHK."""
    spec = contract(mission)
    try:
        rom = rom_path.read_bytes()
    except OSError as exc:
        raise ValueError(f'cannot read authenticated source ROM: {exc}') from exc
    if len(rom) != z8.EXPECTED_ROM_BYTES:
        raise ValueError('source ROM must be exactly 32 MiB')
    rom_sha1 = hashlib.sha1(rom).hexdigest()
    if rom_sha1 != z8.EXPECTED_ROM_SHA1:
        raise ValueError('source ROM SHA-1 differs from the pinned USA revision')

    scenario, _importer, race_pack = z8._source_tools()
    race_pack.verify_extractor_source()
    data = scenario.parse_source(source_root, mission)
    if data['global_catalog_id'] != spec['global_id'] or data['source_map_path'] != spec['map_path']:
        raise ValueError(f'{mission} source map identity differs from its pinned catalog entry')
    if data['source_chk_sha256'] != spec['chk_sha256']:
        raise ValueError(f'{mission} source CHK differs from its pinned SHA-256')

    folder = source_root / mission.lower()
    chk = (folder / 'source.chk').read_bytes()
    units = z8._unit_rows(scenario.parse_chk_sections(chk)[b'UNIT'])
    actions, conditions = data['actions'], data['conditions']
    triggers, locations = data['triggers'], data['locations']
    preserve_count = sum(row['opcode'] == 3 for row in actions)
    create_count = scenario.count_create_unit_actions(actions)
    ai_fourccs = sorted({struct.pack('<I', row['group2']).decode('ascii', 'replace')
                         for row in actions if row['opcode'] == 16})
    counts = {
        'trigger_records': len(triggers), 'actions': len(actions),
        'conditions': len(conditions), 'locations': len(locations),
        'unit_records': len(units), 'preserve_actions': preserve_count,
        'create_actions': create_count,
    }
    if [data['width'], data['height']] != spec['dimensions'] or counts != spec['counts']:
        raise ValueError(f'{mission} source counts differ from authored contract: {counts}')
    for name, rows, key in (('action_histogram', actions, 'opcode'),
                            ('condition_histogram', conditions, 'opcode'),
                            ('unit_histogram', units, 'source_type')):
        if z8._histogram(rows, key) != spec[name]:
            raise ValueError(f'{mission} source {name} differs from contract')
    if ai_fourccs != spec['ai_fourccs']:
        raise ValueError(f'{mission} source AI FourCC set differs from contract')
    if any(row['linked_serial'] for row in units):
        raise ValueError(f'{mission} source UNIT rows unexpectedly carry links')

    manifest = json.loads((folder / 'manifest.json').read_text(encoding='utf-8'))
    if (manifest.get('extractor') != z8.EXTRACTOR_PATH or
            manifest.get('extractor_commit') != z8.EXTRACTOR_COMMIT or
            manifest.get('source_rom_sha1') != z8.EXPECTED_ROM_SHA1 or
            manifest.get('source_chk_sha256') != spec['chk_sha256'] or
            manifest.get('source_map_path') != spec['map_path'] or
            manifest.get('mission_id') != mission):
        raise ValueError(f'{mission} source manifest provenance differs from the authored contract')
    descriptor: dict[str, Any] = {
        'schema': SCHEMA,
        'mission': mission,
        'global_catalog_id': spec['global_id'],
        'source_map_path': spec['map_path'],
        'provenance': {
            'source_rom_sha1': rom_sha1,
            'source_rom_bytes': len(rom),
            'extractor_commit': z8.EXTRACTOR_COMMIT,
            'source_chk_sha256': spec['chk_sha256'],
            'source_chk_bytes': len(chk),
            'sections': manifest['sections'],
        },
        'dimensions': {'width': data['width'], 'height': data['height']},
        'counts': _descriptor_counts(spec),
        'triggers': triggers,
        'conditions': conditions,
        'actions': actions,
        'locations': locations,
        'owners': data['owners'],
        'sides': data['sides'],
        'forces': data['forces'],
        'properties': data['properties'],
        'property_used': data['property_used'],
        'puni': data['puni'],
        'unit_records': units,
        'ai_script_fourccs': ai_fourccs,
    }
    descriptor['descriptor_sha256'] = z8._canonical_fingerprint(descriptor)
    if descriptor['descriptor_sha256'] != spec['descriptor_sha256']:
        raise ValueError(f'canonical {mission} descriptor fingerprint differs from authored '
                         f'contract: {descriptor["descriptor_sha256"]}')
    validate_structure(descriptor)
    return descriptor


def _descriptor_counts(spec: dict[str, Any]) -> dict[str, int]:
    counts = dict(spec['counts'])
    counts['triggers'] = counts.pop('trigger_records')
    return counts


def validate_structure(descriptor: dict[str, Any]) -> None:
    """Reject malformed identities, dimensions, row slices and indexes."""
    if not isinstance(descriptor, dict) or descriptor.get('schema') != SCHEMA:
        raise ValueError('descriptor schema is unsupported')
    mission = descriptor.get('mission')
    if mission not in CONTRACTS:
        raise ValueError('descriptor mission is not a late Zerg mission')
    spec = CONTRACTS[mission]
    if (descriptor.get('global_catalog_id') != spec['global_id'] or
            descriptor.get('source_map_path') != spec['map_path']):
        raise ValueError(f'descriptor identity does not match {mission}')
    counts = descriptor.get('counts')
    if counts != _descriptor_counts(spec):
        raise ValueError(f'descriptor row counts differ from the canonical {mission} contract')
    width, height = spec['dimensions']
    if descriptor.get('dimensions') != {'width': width, 'height': height}:
        raise ValueError(f'descriptor dimensions differ from the canonical {mission} contract')
    map_w, map_h = width * 32, height * 32

    arrays = ('triggers', 'actions', 'conditions', 'locations', 'unit_records',
              'properties', 'property_used', 'puni', 'owners', 'sides', 'forces')
    if any(not isinstance(descriptor.get(name), list) for name in arrays):
        raise ValueError('descriptor contains a missing or non-list row table')
    for name in ('triggers', 'actions', 'conditions', 'locations', 'unit_records'):
        if len(descriptor[name]) != counts[name]:
            raise ValueError(f'descriptor {name} array length disagrees with counts')
    if len(descriptor['properties']) != 64 or any(
            not isinstance(row, list) or len(row) != 20 or
            any(not z8._byte(value) for value in row)
            for row in descriptor['properties']):
        raise ValueError('descriptor UPRP must contain exactly 64 complete byte rows')
    if len(descriptor['property_used']) != 64 or any(not z8._byte(v) for v in descriptor['property_used']):
        raise ValueError('descriptor UPUS must contain 64 bytes')
    if len(descriptor['puni']) != 5700 or any(not z8._byte(v) for v in descriptor['puni']):
        raise ValueError('descriptor PUNI must contain 5700 bytes')
    for name, size in (('owners', 12), ('sides', 12), ('forces', 20)):
        if len(descriptor[name]) != size or any(not z8._byte(v) for v in descriptor[name]):
            raise ValueError(f'descriptor {name} table has invalid bounds')

    location_count = counts['locations']
    for index, location in enumerate(descriptor['locations'], 1):
        if not isinstance(location, dict) or location.get('index') != index:
            raise ValueError('descriptor location indexes are not canonical and contiguous')
        coords = [location.get(key) for key in ('left', 'top', 'right', 'bottom')]
        if any(not z8._uint(value, 32) for value in coords):
            raise ValueError(f'location {index} has an out-of-range coordinate')
        if (coords[0] > coords[2] or coords[1] > coords[3] or
                coords[2] > map_w or coords[3] > map_h):
            raise ValueError(f'location {index} has invalid or out-of-map bounds')

    action_rows, condition_rows = descriptor['actions'], descriptor['conditions']
    for kind, rows in (('action', action_rows), ('condition', condition_rows)):
        for row in rows:
            if not isinstance(row, dict):
                raise ValueError(f'descriptor {kind} row is not an object')
            trigger, row_index = row.get('trigger'), row.get('index')
            if not z8._uint(trigger, 8) or trigger >= counts['triggers'] or not z8._uint(row_index, 8):
                raise ValueError(f'descriptor {kind} row has an invalid trigger/index')
            location = row.get('location')
            if not z8._uint(location, 32) or location > location_count:
                raise ValueError(f'descriptor {kind} row references an invalid location')
            opcode = row.get('opcode')
            limit = 59 if kind == 'action' else 24
            if not z8._uint(opcode, 8) or opcode == 0 or opcode >= limit:
                raise ValueError(f'descriptor {kind} row has an invalid opcode')

    expected_action = expected_condition = 0
    for index, trigger in enumerate(descriptor['triggers']):
        if not isinstance(trigger, dict) or trigger.get('index') != index:
            raise ValueError('descriptor trigger indexes are not canonical and contiguous')
        for prefix, table, limit in (('action', action_rows, 64), ('condition', condition_rows, 16)):
            expected = expected_action if prefix == 'action' else expected_condition
            first, amount = trigger.get(f'first_{prefix}'), trigger.get(f'{prefix}_count')
            if not z8._uint(first, 16) or not z8._uint(amount, 16) or first != expected:
                raise ValueError(f'trigger {index} {prefix} range is not contiguous')
            end = first + amount
            if end > len(table) or any(row['trigger'] != index or row['index'] >= limit
                                       for row in table[first:end]):
                raise ValueError(f'trigger {index} {prefix} range disagrees with its rows')
            if prefix == 'action':
                expected_action = end
            else:
                expected_condition = end
        flags, cursor = trigger.get('execution_flags'), trigger.get('initial_cursor')
        enabled = trigger.get('enabled_players')
        if not z8._uint(flags, 32) or not z8._uint(cursor, 8) or cursor >= 64:
            raise ValueError(f'trigger {index} metadata is out of range')
        if not isinstance(enabled, list) or len(enabled) != 27 or any(v not in (0, 1) for v in enabled):
            raise ValueError(f'trigger {index} enabled-player table is malformed')
    if expected_action != len(action_rows) or expected_condition != len(condition_rows):
        raise ValueError('trigger row slices do not cover the complete tables')

    units = descriptor['unit_records']
    for index, row in enumerate(units):
        if not isinstance(row, dict) or row.get('record_index') != index:
            raise ValueError('descriptor UNIT record indexes are not contiguous')
        for key, bits in (('serial', 32), ('linked_serial', 32), ('source_type', 16),
                          ('owner_raw', 8), ('x_source_px', 16), ('y_source_px', 16),
                          ('relation', 16), ('special', 16), ('valid', 16),
                          ('hp_percent', 8), ('shield_percent', 8),
                          ('energy_percent', 8), ('resources', 32),
                          ('hangar', 16), ('flags', 16)):
            if not z8._uint(row.get(key), bits):
                raise ValueError(f'UNIT row {index} {key} is out of range')
        if row['x_source_px'] >= map_w or row['y_source_px'] >= map_h:
            raise ValueError(f'UNIT row {index} is outside the {mission} map bounds')
        if row['linked_serial'] or row.get('linked_record_index') is not None:
            raise ValueError(f'UNIT row {index} carries an unexpected link')
    if len({row['serial'] for row in units}) != len(units):
        raise ValueError('descriptor UNIT serials are not unique')

    fingerprint = descriptor.get('descriptor_sha256')
    if not isinstance(fingerprint, str) or fingerprint != z8._canonical_fingerprint(descriptor):
        raise ValueError('descriptor fingerprint does not match its complete JSON rows')


def require_authenticated(descriptor: dict[str, Any]) -> None:
    """Structure check plus the pinned per-mission source fingerprint."""
    validate_structure(descriptor)
    mission = descriptor['mission']
    if descriptor['descriptor_sha256'] != CONTRACTS[mission]['descriptor_sha256']:
        raise ValueError(f'descriptor fingerprint is not the authenticated {mission} source')


def validate_descriptor_file(mission: str, descriptor_path: Path,
                             source_root: Path = DEFAULT_SOURCE_ROOT,
                             rom_path: Path = DEFAULT_ROM) -> dict[str, Any]:
    expected = build_descriptor(mission, source_root, rom_path)
    try:
        candidate = json.loads(descriptor_path.read_text(encoding='utf-8'),
                               object_pairs_hook=z8._unique_json_object)
    except (OSError, json.JSONDecodeError) as exc:
        raise ValueError(f'cannot read descriptor JSON: {exc}') from exc
    require_authenticated(candidate)
    if candidate != expected:
        raise ValueError(f'descriptor rows or provenance differ from authenticated {mission} source')
    return expected


def _private_output_path(mission: str, path: Path) -> Path:
    # Same lexical, symlink-refusing rule as the Z8 pack, scoped per mission.
    prefix = mission.lower() + '-'
    absolute = Path(os.path.abspath(path))
    private_root = Path(os.path.abspath(ROOT / '.local/build'))
    try:
        relative = absolute.relative_to(private_root)
    except ValueError as exc:
        raise ValueError(f'output must be under ignored .local/build/{prefix}*') from exc
    if not relative.parts or not relative.parts[0].startswith(prefix):
        raise ValueError(f'output must be under an ignored .local/build/{prefix}* directory')
    current = private_root
    for component in relative.parts:
        current = current / component
        if current.is_symlink():
            raise ValueError('refusing output through a symlink path')
    if absolute.exists():
        raise ValueError('refusing to overwrite an existing private descriptor')
    return absolute


def write_private_descriptor(descriptor: dict[str, Any], output: Path) -> dict[str, Any]:
    require_authenticated(descriptor)
    output = _private_output_path(descriptor['mission'], output)
    output.parent.mkdir(parents=True, exist_ok=True)
    payload = json.dumps(descriptor, sort_keys=True, indent=2, ensure_ascii=True) + '\n'
    try:
        with output.open('x', encoding='ascii') as stream:
            stream.write(payload)
    except FileExistsError as exc:
        raise ValueError('refusing to overwrite an existing private descriptor') from exc
    return {'descriptor': str(output), 'descriptor_sha256': descriptor['descriptor_sha256'],
            'descriptor_bytes': len(payload.encode('ascii'))}


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest='command', required=True)
    for name, text in (('pack', 'validate source and write a private descriptor'),
                       ('verify', 'compare a private descriptor to source')):
        cmd = sub.add_parser(name, help=text)
        cmd.add_argument('--mission', choices=sorted(CONTRACTS), required=True)
        cmd.add_argument('--source-root', type=Path, default=DEFAULT_SOURCE_ROOT)
        cmd.add_argument('--rom', type=Path, default=DEFAULT_ROM)
        if name == 'pack':
            cmd.add_argument('--out', type=Path, required=True)
        else:
            cmd.add_argument('--descriptor', type=Path, required=True)
    args = parser.parse_args(argv)
    try:
        if args.command == 'pack':
            descriptor = build_descriptor(args.mission, args.source_root, args.rom)
            result = write_private_descriptor(descriptor, args.out)
        else:
            descriptor = validate_descriptor_file(args.mission, args.descriptor,
                                                  args.source_root, args.rom)
            result = {'descriptor': str(args.descriptor),
                      'descriptor_sha256': descriptor['descriptor_sha256'],
                      'validated': True}
        print(json.dumps(result, indent=2, sort_keys=True))
        return 0
    except (OSError, ValueError, KeyError, RuntimeError) as exc:
        print(f'sc64_zerg_late_pack: {exc}', file=sys.stderr)
        return 1


if __name__ == '__main__':
    raise SystemExit(main())
