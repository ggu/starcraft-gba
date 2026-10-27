#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Authenticate and privately inventory the Z8 source scenario.

This source-only tool never invokes the importer or regenerates shared banks.
It consumes a previously retained Z8 CHK, authenticates its manifest and ROM,
then writes a complete numeric descriptor only to an explicit ignored
``.local/build/z8-*`` path. It does not implement or enable mission runtime
semantics.
"""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import struct
import sys
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_SOURCE_ROOT = ROOT / '.local/sc64/zp-stage/generated/source_maps'
DEFAULT_ROM = ROOT / '.local/sc64/rom/StarCraft64-USA.z64'
EXPECTED_ROM_SHA1 = 'a36d91b5aac7bb95df0d7de5f9814f5270e25c2b'
EXPECTED_ROM_BYTES = 32 * 1024 * 1024
EXPECTED_CHK_SHA256 = 'ff6d0177a6a76fc68ef98f0962661857853ea11c50de9060b90ff934826cfd8e'
EXPECTED_DESCRIPTOR_SHA256 = 'f80b5f9e9419c324a6b86b9a11bbe3dab0ec545d20817fb0f26509e82c85a986'
EXTRACTOR_COMMIT = '09b57f787a247107d1de82b8c5eab2734667ea1d'
EXTRACTOR_PATH = 'references/sc64-maps/extract_sc64_maps.py'
MAP_PATH = '008/01B'
GLOBAL_ID = 19
SCHEMA = 1
SC64_Z8_MAP_PIXEL_WIDTH = 128 * 32
SC64_Z8_MAP_PIXEL_HEIGHT = 128 * 32

EXPECTED_COUNTS = {
    'dimensions': [128, 128],
    'trigger_records': 39,
    'actions': 122,
    'conditions': 55,
    'locations': 64,
    'unit_records': 198,
    'preserve_actions': 13,
    'create_actions': 16,
    'nydus_records': 4,
}
EXPECTED_ACTION_HISTOGRAM = {
    1: 1, 2: 5, 3: 13, 4: 10, 7: 6, 8: 6, 9: 6, 10: 5, 11: 16,
    12: 1, 13: 34, 16: 8, 26: 3, 28: 3, 38: 2, 43: 3,
}
EXPECTED_CONDITION_HISTOGRAM = {2: 14, 3: 9, 5: 1, 11: 14, 12: 17}
EXPECTED_UNIT_HISTOGRAM = {
    37: 9, 38: 6, 39: 2, 41: 2, 42: 5, 51: 1, 64: 9, 65: 9,
    66: 13, 67: 1, 70: 6, 83: 4, 84: 1, 131: 3, 134: 4,
    143: 1, 154: 2, 155: 2, 156: 26, 159: 2, 160: 3, 162: 16,
    163: 1, 164: 2, 165: 1, 166: 1, 167: 1, 171: 2, 176: 10,
    177: 24, 178: 17, 188: 5, 194: 3, 214: 4,
}
EXPECTED_AI_FOURCCS = ['MvTe', 'Ze8T', 'Zer8']


def _load_module(name: str, path: Path):
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f'cannot load source parser: {path}')
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


def _source_tools():
    scenario = _load_module('sc64_z8_source_scenario_pack',
                            ROOT / 'tools/sc64_zp_scenario_pack.py')
    importer = _load_module('sc64_z8_source_import', ROOT / 'tools/sc64_import.py')
    race_pack = _load_module('sc64_z8_source_catalog', ROOT / 'tools/sc64_races_pack.py')
    return scenario, importer, race_pack


def _hash(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def _canonical_fingerprint(descriptor: dict[str, Any]) -> str:
    payload = {key: value for key, value in descriptor.items()
               if key != 'descriptor_sha256'}
    encoded = json.dumps(payload, sort_keys=True, separators=(',', ':'),
                         ensure_ascii=True).encode('ascii')
    return hashlib.sha256(encoded).hexdigest()


def _unit_rows(payload: bytes) -> list[dict[str, Any]]:
    """Retain every 36-byte UNIT field, including stable serial link IDs."""
    if len(payload) % 36:
        raise ValueError('UNIT section is not a multiple of 36-byte records')
    rows: list[dict[str, Any]] = []
    for index, offset in enumerate(range(0, len(payload), 36)):
        raw = payload[offset:offset + 36]
        x, y, raw_type = struct.unpack_from('<HHH', raw, 4)
        relation, special, valid = struct.unpack_from('<HHH', raw, 10)
        rows.append({
            'record_index': index,
            'serial': struct.unpack_from('<I', raw, 0)[0],
            'x_source_px': x,
            'y_source_px': y,
            'source_type': raw_type,
            'relation': relation,
            'special': special,
            'valid': valid,
            'owner_raw': raw[16],
            'hp_percent': raw[17],
            'shield_percent': raw[18],
            'energy_percent': raw[19],
            'resources': struct.unpack_from('<I', raw, 20)[0],
            'hangar': struct.unpack_from('<H', raw, 24)[0],
            'flags': struct.unpack_from('<H', raw, 26)[0],
            'unused_hex': raw[28:32].hex(),
            'linked_serial': struct.unpack_from('<I', raw, 32)[0],
        })
    serial_to_index = {row['serial']: row['record_index'] for row in rows}
    if len(serial_to_index) != len(rows):
        raise ValueError('UNIT source serial IDs are not unique')
    for row in rows:
        linked = row['linked_serial']
        if linked and linked not in serial_to_index:
            raise ValueError(f"UNIT row {row['record_index']} links to absent serial {linked:#x}")
        row['linked_record_index'] = serial_to_index.get(linked) if linked else None
    return rows


def build_descriptor(source_root: Path = DEFAULT_SOURCE_ROOT,
                     rom_path: Path = DEFAULT_ROM) -> dict[str, Any]:
    """Build the canonical private JSON descriptor from already staged Z8."""
    try:
        rom = rom_path.read_bytes()
    except OSError as exc:
        raise ValueError(f'cannot read authenticated source ROM: {exc}') from exc
    if len(rom) != EXPECTED_ROM_BYTES:
        raise ValueError('source ROM must be exactly 32 MiB')
    rom_sha1 = hashlib.sha1(rom).hexdigest()
    if rom_sha1 != EXPECTED_ROM_SHA1:
        raise ValueError('source ROM SHA-1 differs from the pinned USA revision')

    scenario, _importer, race_pack = _source_tools()
    race_pack.verify_extractor_source()
    data = scenario.parse_source(source_root, 'Z8')
    if data['global_catalog_id'] != GLOBAL_ID or data['source_map_path'] != MAP_PATH:
        raise ValueError('Z8 source map identity differs from its pinned catalog entry')
    if data['source_chk_sha256'] != EXPECTED_CHK_SHA256:
        raise ValueError('Z8 source CHK differs from its pinned SHA-256')

    chk = (source_root / 'z8/source.chk').read_bytes()
    sections = scenario.parse_chk_sections(chk)
    units = _unit_rows(sections[b'UNIT'])
    actions = data['actions']
    conditions = data['conditions']
    triggers = data['triggers']
    locations = data['locations']
    action_histogram = _histogram(actions, 'opcode')
    condition_histogram = _histogram(conditions, 'opcode')
    unit_histogram = _histogram(units, 'source_type')
    nydus = [row for row in units if row['source_type'] == 134]
    preserve_count = sum(row['opcode'] == 3 for row in actions)
    create_count = scenario.count_create_unit_actions(actions)
    ai_fourccs = sorted({struct.pack('<I', row['group2']).decode('ascii', 'replace')
                         for row in actions if row['opcode'] == 16})

    counts = {
        'dimensions': [data['width'], data['height']],
        'trigger_records': len(triggers),
        'actions': len(actions),
        'conditions': len(conditions),
        'locations': len(locations),
        'unit_records': len(units),
        'preserve_actions': preserve_count,
        'create_actions': create_count,
        'nydus_records': len(nydus),
    }
    if counts != EXPECTED_COUNTS:
        raise ValueError(f'Z8 source counts differ from authored contract: {counts}')
    if action_histogram != EXPECTED_ACTION_HISTOGRAM:
        raise ValueError('Z8 source action opcode histogram differs from contract')
    if condition_histogram != EXPECTED_CONDITION_HISTOGRAM:
        raise ValueError('Z8 source condition opcode histogram differs from contract')
    if unit_histogram != EXPECTED_UNIT_HISTOGRAM:
        raise ValueError('Z8 source raw UNIT histogram differs from contract')
    if ai_fourccs != EXPECTED_AI_FOURCCS:
        raise ValueError('Z8 source AI FourCC set differs from contract')

    manifest = json.loads((source_root / 'z8/manifest.json').read_text(encoding='utf-8'))
    if (manifest.get('extractor') != EXTRACTOR_PATH or
            manifest.get('extractor_commit') != EXTRACTOR_COMMIT or
            manifest.get('source_rom_sha1') != EXPECTED_ROM_SHA1 or
            manifest.get('source_chk_sha256') != EXPECTED_CHK_SHA256 or
            manifest.get('source_map_path') != MAP_PATH or
            manifest.get('mission_id') != 'Z8'):
        raise ValueError('Z8 source manifest provenance differs from the authored contract')
    section_hashes = manifest['sections']
    descriptor: dict[str, Any] = {
        'schema': SCHEMA,
        'mission': 'Z8',
        'global_catalog_id': GLOBAL_ID,
        'source_map_path': MAP_PATH,
        'provenance': {
            'source_rom_sha1': rom_sha1,
            'source_rom_bytes': len(rom),
            'extractor_commit': EXTRACTOR_COMMIT,
            'source_chk_sha256': EXPECTED_CHK_SHA256,
            'source_chk_bytes': len(chk),
            'sections': section_hashes,
        },
        'dimensions': {'width': data['width'], 'height': data['height']},
        'counts': {
            'triggers': len(triggers), 'actions': len(actions),
            'conditions': len(conditions), 'locations': len(locations),
            'unit_records': len(units), 'preserve_actions': preserve_count,
            'create_actions': create_count, 'nydus_records': len(nydus),
        },
        'histograms': {
            'action_opcode': {str(key): value for key, value in action_histogram.items()},
            'condition_opcode': {str(key): value for key, value in condition_histogram.items()},
            'unit_raw_type': {str(key): value for key, value in unit_histogram.items()},
        },
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
        'ai_script_rows': [row for row in actions if row['opcode'] == 16],
        'ai_script_fourccs': ai_fourccs,
        'nydus_pairs': _nydus_pairs(nydus),
    }
    descriptor['descriptor_sha256'] = _canonical_fingerprint(descriptor)
    if descriptor['descriptor_sha256'] != EXPECTED_DESCRIPTOR_SHA256:
        raise ValueError('canonical Z8 descriptor fingerprint differs from authored contract')
    validate_structure(descriptor)
    return descriptor


def _histogram(rows: list[dict[str, Any]], key: str) -> dict[int, int]:
    result: dict[int, int] = {}
    for row in rows:
        value = row[key]
        result[value] = result.get(value, 0) + 1
    return dict(sorted(result.items()))


def _nydus_pairs(rows: list[dict[str, Any]]) -> list[list[int]]:
    pairs = set()
    for row in rows:
        linked = row['linked_record_index']
        if linked is None:
            raise ValueError(f"Nydus UNIT row {row['record_index']} has no linked partner")
        pair = sorted((row['record_index'], linked))
        pairs.add(tuple(pair))
    return [list(pair) for pair in sorted(pairs)]


def validate_structure(descriptor: dict[str, Any]) -> None:
    """Reject malformed dimensions, row slices, indexes, and links."""
    if not isinstance(descriptor, dict) or descriptor.get('schema') != SCHEMA:
        raise ValueError('descriptor schema is unsupported')
    for key, expected in (('mission', 'Z8'), ('global_catalog_id', GLOBAL_ID),
                          ('source_map_path', MAP_PATH)):
        if descriptor.get(key) != expected:
            raise ValueError(f'descriptor {key} does not match Z8 identity')
    counts = descriptor.get('counts')
    if not isinstance(counts, dict):
        raise ValueError('descriptor counts are missing')
    expected_count_keys = {
        'triggers': EXPECTED_COUNTS['trigger_records'],
        'actions': EXPECTED_COUNTS['actions'],
        'conditions': EXPECTED_COUNTS['conditions'],
        'locations': EXPECTED_COUNTS['locations'],
        'unit_records': EXPECTED_COUNTS['unit_records'],
        'preserve_actions': EXPECTED_COUNTS['preserve_actions'],
        'create_actions': EXPECTED_COUNTS['create_actions'],
        'nydus_records': EXPECTED_COUNTS['nydus_records'],
    }
    if counts != expected_count_keys:
        raise ValueError('descriptor row counts differ from the canonical Z8 contract')
    dims = descriptor.get('dimensions')
    if dims != {'width': 128, 'height': 128}:
        raise ValueError('descriptor dimensions differ from the canonical Z8 contract')

    arrays = ('triggers', 'actions', 'conditions', 'locations', 'unit_records',
              'properties', 'property_used', 'puni', 'owners', 'sides', 'forces')
    if any(not isinstance(descriptor.get(name), list) for name in arrays):
        raise ValueError('descriptor contains a missing or non-list row table')
    for name, count_key in (('triggers', 'triggers'), ('actions', 'actions'),
                            ('conditions', 'conditions'), ('locations', 'locations'),
                            ('unit_records', 'unit_records')):
        if len(descriptor[name]) != counts[count_key]:
            raise ValueError(f'descriptor {name} array length disagrees with counts')
    if len(descriptor['properties']) != 64 or any(
            not isinstance(row, list) or len(row) != 20 or
            any(not _byte(value) for value in row)
            for row in descriptor['properties']):
        raise ValueError('descriptor UPRP must contain exactly 64 complete byte rows')
    if len(descriptor['property_used']) != 64 or any(not _byte(v) for v in descriptor['property_used']):
        raise ValueError('descriptor UPUS must contain 64 bytes')
    if len(descriptor['puni']) != 5700 or any(not _byte(v) for v in descriptor['puni']):
        raise ValueError('descriptor PUNI must contain 5700 bytes')
    for name, size in (('owners', 12), ('sides', 12), ('forces', 20)):
        if len(descriptor[name]) != size or any(not _byte(v) for v in descriptor[name]):
            raise ValueError(f'descriptor {name} table has invalid bounds')

    location_count = counts['locations']
    for index, location in enumerate(descriptor['locations'], 1):
        if not isinstance(location, dict) or location.get('index') != index:
            raise ValueError('descriptor location indexes are not canonical and contiguous')
        coords = [location.get(key) for key in ('left', 'top', 'right', 'bottom')]
        if any(not _uint(value, 32) for value in coords):
            raise ValueError(f'location {index} has an out-of-range coordinate')
        if (coords[0] > coords[2] or coords[1] > coords[3] or
                coords[2] > SC64_Z8_MAP_PIXEL_WIDTH or
                coords[3] > SC64_Z8_MAP_PIXEL_HEIGHT):
            raise ValueError(f'location {index} has invalid or out-of-map bounds')

    action_rows = descriptor['actions']
    condition_rows = descriptor['conditions']
    for kind, rows, loc_limit in (('action', action_rows, location_count),
                                  ('condition', condition_rows, location_count)):
        for row in rows:
            if not isinstance(row, dict):
                raise ValueError(f'descriptor {kind} row is not an object')
            trigger, row_index = row.get('trigger'), row.get('index')
            if not _uint(trigger, 8) or trigger >= counts['triggers'] or not _uint(row_index, 8):
                raise ValueError(f'descriptor {kind} row has an invalid trigger/index')
            location = row.get('location')
            if not _uint(location, 32) or location > loc_limit:
                raise ValueError(f'descriptor {kind} row references an invalid location')
            opcode = row.get('opcode')
            limit = 59 if kind == 'action' else 24
            if not _uint(opcode, 8) or opcode == 0 or opcode >= limit:
                raise ValueError(f'descriptor {kind} row has an invalid opcode')

    expected_action_offset = expected_condition_offset = 0
    for index, trigger in enumerate(descriptor['triggers']):
        if not isinstance(trigger, dict) or trigger.get('index') != index:
            raise ValueError('descriptor trigger indexes are not canonical and contiguous')
        for prefix, expected_offset, table, limit in (
                ('action', expected_action_offset, action_rows, 64),
                ('condition', expected_condition_offset, condition_rows, 16)):
            first = trigger.get(f'first_{prefix}')
            amount = trigger.get(f'{prefix}_count')
            if not _uint(first, 16) or not _uint(amount, 16) or first != expected_offset:
                raise ValueError(f'trigger {index} {prefix} range is not contiguous')
            end = first + amount
            if end > len(table):
                raise ValueError(f'trigger {index} {prefix} range exceeds descriptor bounds')
            if any(row['trigger'] != index or row['index'] >= limit
                   for row in table[first:end]):
                raise ValueError(f'trigger {index} {prefix} range disagrees with its rows')
            if prefix == 'action':
                expected_action_offset = end
            else:
                expected_condition_offset = end
        flags = trigger.get('execution_flags')
        cursor = trigger.get('initial_cursor')
        enabled = trigger.get('enabled_players')
        if not _uint(flags, 32) or not _uint(cursor, 8) or cursor >= 64:
            raise ValueError(f'trigger {index} metadata is out of range')
        if not isinstance(enabled, list) or len(enabled) != 27 or any(v not in (0, 1) for v in enabled):
            raise ValueError(f'trigger {index} enabled-player table is malformed')
    if expected_action_offset != len(action_rows) or expected_condition_offset != len(condition_rows):
        raise ValueError('trigger row slices do not cover the complete tables')

    units = descriptor['unit_records']
    for index, row in enumerate(units):
        if not isinstance(row, dict):
            raise ValueError(f'UNIT row {index} is not an object')
        if row.get('record_index') != index:
            raise ValueError('descriptor UNIT record indexes are not contiguous')
        if not _uint(row.get('serial'), 32):
            raise ValueError(f'UNIT row {index} serial is out of range')
    if len({row['serial'] for row in units}) != len(units):
        raise ValueError('descriptor UNIT serials are not unique')
    serial_to_index = {row['serial']: row['record_index'] for row in units}
    for row in units:
        for key, bits in (('serial', 32), ('linked_serial', 32), ('source_type', 16),
                          ('owner_raw', 8), ('x_source_px', 16), ('y_source_px', 16),
                          ('relation', 16), ('special', 16), ('valid', 16),
                          ('hp_percent', 8), ('shield_percent', 8),
                          ('energy_percent', 8), ('resources', 32),
                          ('hangar', 16), ('flags', 16)):
            if not _uint(row.get(key), bits):
                raise ValueError(f"UNIT row {row.get('record_index')} {key} is out of range")
        if (row['x_source_px'] >= SC64_Z8_MAP_PIXEL_WIDTH or
                row['y_source_px'] >= SC64_Z8_MAP_PIXEL_HEIGHT):
            raise ValueError(f"UNIT row {row['record_index']} is outside the Z8 map bounds")
        linked = row['linked_serial']
        if linked and (linked not in serial_to_index or
                       row['linked_record_index'] != serial_to_index[linked]):
            raise ValueError(f"UNIT row {row['record_index']} has an invalid linked serial")
        if linked and units[serial_to_index[linked]]['linked_serial'] != row['serial']:
            raise ValueError(f"UNIT row {row['record_index']} link is not reciprocal")
        if not linked and row['linked_record_index'] is not None:
            raise ValueError(f"UNIT row {row['record_index']} has a spurious link index")
    nydus = [row for row in units if row['source_type'] == 134]
    if len(nydus) != EXPECTED_COUNTS['nydus_records'] or _nydus_pairs(nydus) != [[192, 194], [195, 197]]:
        raise ValueError('descriptor Nydus links differ from the authenticated pair contract')

    fingerprint = descriptor.get('descriptor_sha256')
    if not isinstance(fingerprint, str) or fingerprint != _canonical_fingerprint(descriptor):
        raise ValueError('descriptor fingerprint does not match its complete JSON rows')


def _uint(value: Any, bits: int) -> bool:
    return isinstance(value, int) and not isinstance(value, bool) and 0 <= value < (1 << bits)


def _byte(value: Any) -> bool:
    return _uint(value, 8)


def validate_descriptor_file(descriptor_path: Path, source_root: Path = DEFAULT_SOURCE_ROOT,
                              rom_path: Path = DEFAULT_ROM) -> dict[str, Any]:
    expected = build_descriptor(source_root, rom_path)
    try:
        candidate = json.loads(descriptor_path.read_text(encoding='utf-8'),
                               object_pairs_hook=_unique_json_object)
    except (OSError, json.JSONDecodeError) as exc:
        raise ValueError(f'cannot read descriptor JSON: {exc}') from exc
    validate_candidate(candidate, expected)
    return expected


def _unique_json_object(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    """Build one JSON object while rejecting duplicate keys at every depth."""
    result: dict[str, Any] = {}
    for key, value in pairs:
        if key in result:
            raise ValueError(f'descriptor JSON contains duplicate key {key!r}')
        result[key] = value
    return result


def validate_candidate(candidate: dict[str, Any], expected: dict[str, Any]) -> None:
    """Validate structure and require every field to match trusted source rows."""
    validate_structure(candidate)
    if candidate != expected:
        raise ValueError('descriptor rows or provenance differ from authenticated Z8 source')


def _private_output_path(path: Path) -> Path:
    # Keep the lexical path so symlinks cannot redirect an apparently private
    # output elsewhere (or alias a second private output). Resolve `..` without
    # following symlinks, then reject each existing symlink component.
    absolute = Path(os.path.abspath(path))
    private_root = Path(os.path.abspath(ROOT / '.local/build'))
    try:
        relative = absolute.relative_to(private_root)
    except ValueError as exc:
        raise ValueError('output must be under ignored .local/build/z8-*') from exc
    if not relative.parts or not relative.parts[0].startswith('z8-'):
        raise ValueError('output must be under an ignored .local/build/z8-* directory')
    current = ROOT
    for component in private_root.relative_to(ROOT).parts + relative.parts:
        current = current / component
        if current.is_symlink():
            raise ValueError('refusing output through a symlink path')
    if absolute.exists():
        raise ValueError('refusing to overwrite an existing private descriptor')
    return absolute


def write_private_descriptor(descriptor: dict[str, Any], output: Path) -> dict[str, Any]:
    output = _private_output_path(output)
    validate_structure(descriptor)
    if descriptor.get('descriptor_sha256') != EXPECTED_DESCRIPTOR_SHA256:
        raise ValueError('refusing to write a descriptor outside the canonical Z8 fingerprint')
    output.parent.mkdir(parents=True, exist_ok=True)
    payload = json.dumps(descriptor, sort_keys=True, indent=2, ensure_ascii=True) + '\n'
    try:
        # The path check is advisory; exclusive creation closes the race where
        # another process creates the target after that check.
        with output.open('x', encoding='ascii') as stream:
            stream.write(payload)
    except FileExistsError as exc:
        raise ValueError('refusing to overwrite an existing private descriptor') from exc
    return {'descriptor': str(output), 'descriptor_sha256': descriptor['descriptor_sha256'],
            'descriptor_bytes': len(payload.encode('ascii'))}


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest='command', required=True)
    pack = sub.add_parser('pack', help='validate source and write a private descriptor')
    pack.add_argument('--source-root', type=Path, default=DEFAULT_SOURCE_ROOT)
    pack.add_argument('--rom', type=Path, default=DEFAULT_ROM)
    pack.add_argument('--out', type=Path, required=True)
    verify = sub.add_parser('verify', help='compare a private descriptor to source')
    verify.add_argument('--source-root', type=Path, default=DEFAULT_SOURCE_ROOT)
    verify.add_argument('--rom', type=Path, default=DEFAULT_ROM)
    verify.add_argument('--descriptor', type=Path, required=True)
    args = parser.parse_args(argv)
    try:
        if args.command == 'pack':
            descriptor = build_descriptor(args.source_root, args.rom)
            result = write_private_descriptor(descriptor, args.out)
        else:
            descriptor = validate_descriptor_file(args.descriptor, args.source_root, args.rom)
            result = {'descriptor': str(args.descriptor),
                      'descriptor_sha256': descriptor['descriptor_sha256'],
                      'source_chk_sha256': EXPECTED_CHK_SHA256,
                      'validated': True}
        print(json.dumps(result, indent=2, sort_keys=True))
        return 0
    except (OSError, ValueError, KeyError, RuntimeError) as exc:
        print(f'sc64_z8_pack: {exc}', file=sys.stderr)
        return 1


if __name__ == '__main__':
    raise SystemExit(main())
