#!/usr/bin/env python3
"""Compile private T4 location, trigger and THG2 records into immutable C data.

Source CHK records and generated headers belong under ignored .local storage.
This tool contains only schema and validation logic, never scenario operands or
text assets.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import struct
import tempfile


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_BASE = ROOT / '.local/sc64/races/generated/t4'
DEFAULT_RECORDS = DEFAULT_BASE / 'records'
DEFAULT_MANIFEST = DEFAULT_BASE / 'manifest.json'
DEFAULT_OUTPUT = ROOT / '.local/sc64/races/generated/sc64_t4_scenario_generated.h'
VERIFIED_ROM_SHA1 = 'a36d91b5aac7bb95df0d7de5f9814f5270e25c2b'
REQUIRED = {
    'MRGN': ('MRGN.bin', b'MRGN', 20),
    'TRIG': ('TRIG.bin', b'TRIG', 2400),
    'THG2': ('THG2.bin', b'THG2', 10),
    'FORC': ('FORC.bin', b'FORC', 20),
    'OWNR': ('OWNR.bin', b'OWNR', 12),
    'SIDE': ('SIDE.bin', b'SIDE', 12),
    'UPRP': ('UPRP.bin', b'UPRP', 20),
    'UPUS': ('UPUS.bin', b'UPUS', 64),
}
CONDITIONS_PER_TRIGGER = 16
ACTIONS_PER_TRIGGER = 64
TRIGGER_ENABLED_BYTES = 27


def _sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def _framed_record(data: bytes, expected_tag: bytes, label: str) -> bytes:
    if len(data) < 8:
        raise ValueError(f'{label}: truncated CHK section header')
    tag = data[:4]
    size = struct.unpack_from('<I', data, 4)[0]
    if tag != expected_tag:
        raise ValueError(f'{label}: expected {expected_tag!r}, found {tag!r}')
    if size != len(data) - 8:
        raise ValueError(f'{label}: framed payload length does not match file')
    return data[8:]


def parse_source(records_dir: Path = DEFAULT_RECORDS,
                 manifest_path: Path = DEFAULT_MANIFEST, *,
                 expected_locations: int = 255,
                 expected_triggers: int = 38,
                 expected_doodads: int = 32,
                 expected_properties: int = 64,
                 expected_active_conditions: int | None = 48,
                 expected_active_actions: int | None = 141) -> dict:
    """Read, authenticate and decode exact private framed CHK records."""
    try:
        manifest = json.loads(manifest_path.read_text(encoding='utf-8'))
    except (OSError, json.JSONDecodeError) as exc:
        raise ValueError(f'cannot read T4 source manifest: {exc}') from exc
    if manifest.get('schema') != 1 or manifest.get('source_map_path') != '008/00D':
        raise ValueError('manifest does not describe the expected T4 source map')
    if manifest.get('source_rom_sha1') != VERIFIED_ROM_SHA1:
        raise ValueError('manifest ROM is not the verified StarCraft 64 USA image')
    for key in ('source_chk_sha256', 'extractor_commit'):
        if not isinstance(manifest.get(key), str) or not manifest[key]:
            raise ValueError(f'manifest is missing {key}')
    if len(manifest['source_chk_sha256']) != 64:
        raise ValueError('manifest source CHK hash is malformed')
    records = manifest.get('records')
    if not isinstance(records, dict):
        raise ValueError('manifest records entry is missing')

    payloads: dict[str, bytes] = {}
    framed: dict[str, bytes] = {}
    for name, (filename, tag, _) in REQUIRED.items():
        metadata = records.get(name)
        if not isinstance(metadata, dict) or metadata.get('file') != filename:
            raise ValueError(f'manifest has no valid {name} record entry')
        try:
            data = (records_dir / filename).read_bytes()
        except OSError as exc:
            raise ValueError(f'cannot read {name} record: {exc}') from exc
        payload = _framed_record(data, tag, name)
        if metadata.get('record_bytes') != len(data) or metadata.get('payload_bytes') != len(payload):
            raise ValueError(f'{name} record size disagrees with its manifest')
        if metadata.get('record_sha256') != _sha256(data):
            raise ValueError(f'{name} record integrity check failed')
        if metadata.get('payload_sha256') != _sha256(payload):
            raise ValueError(f'{name} payload integrity check failed')
        if len(payload) % REQUIRED[name][2] != 0:
            raise ValueError(f'{name} payload has a partial source record')
        payloads[name] = payload
        framed[name] = data

    location_payload = payloads['MRGN']
    trigger_payload = payloads['TRIG']
    doodad_payload = payloads['THG2']
    location_count = len(location_payload) // REQUIRED['MRGN'][2]
    trigger_count = len(trigger_payload) // REQUIRED['TRIG'][2]
    doodad_count = len(doodad_payload) // REQUIRED['THG2'][2]
    if location_count != expected_locations:
        raise ValueError(f'MRGN has {location_count} locations, expected {expected_locations}')
    if trigger_count != expected_triggers:
        raise ValueError(f'TRIG has {trigger_count} triggers, expected {expected_triggers}')
    if doodad_count != expected_doodads:
        raise ValueError(f'THG2 has {doodad_count} doodads, expected {expected_doodads}')
    for name, length in (('FORC', 20), ('OWNR', 12), ('SIDE', 12), ('UPUS', 64)):
        if len(payloads[name]) != length:
            raise ValueError(f'{name} must have exactly {length} source bytes')
    if len(payloads['UPRP']) != expected_properties * 20:
        raise ValueError(f'UPRP must have exactly {expected_properties} property records')
    if any(force > 3 for force in payloads['FORC'][:8]):
        raise ValueError('FORC has an invalid player force index')
    if any(used > 1 for used in payloads['UPUS']):
        raise ValueError('UPUS has an invalid property-used byte')

    locations = []
    for offset in range(0, len(location_payload), 20):
        raw = location_payload[offset:offset + 20]
        left, top, right, bottom, name_id, elevation = struct.unpack('<IIIIHH', raw)
        locations.append({'left': left, 'top': top, 'right': right, 'bottom': bottom,
                          'name_id': name_id, 'elevation': elevation, 'raw': raw})

    conditions = []
    actions = []
    triggers = []
    active_conditions = active_actions = 0
    for trigger_index in range(trigger_count):
        start = trigger_index * 2400
        raw_trigger = trigger_payload[start:start + 2400]
        condition_row = []
        for slot in range(CONDITIONS_PER_TRIGGER):
            raw = raw_trigger[slot * 20:(slot + 1) * 20]
            fields = struct.unpack('<IIIHBBBBH', raw)
            row = {'location': fields[0], 'player': fields[1], 'amount': fields[2],
                   'unit': fields[3], 'comparison': fields[4], 'opcode': fields[5],
                   'resource': fields[6], 'flags': fields[7], 'mask': fields[8]}
            condition_row.append(row)
            if row['opcode']:
                active_conditions += 1
            conditions.append(row)
        action_row = []
        action_base = 16 * 20
        for slot in range(ACTIONS_PER_TRIGGER):
            raw = raw_trigger[action_base + slot * 32:action_base + (slot + 1) * 32]
            fields = struct.unpack('<IIIIIIHBBB3s', raw)
            row = {'location': fields[0], 'string_id': fields[1], 'wav_id': fields[2],
                   'time': fields[3], 'player': fields[4], 'argument': fields[5],
                   'unit': fields[6], 'opcode': fields[7], 'modifier': fields[8],
                   'flags': fields[9], 'trailing': fields[10]}
            action_row.append(row)
            if row['opcode']:
                active_actions += 1
            actions.append(row)
        # Trigger header tail is execution_flags:u32, enabled-player bytes
        # [27], then the initial action cursor:u8.
        execution_flags = struct.unpack_from('<I', raw_trigger, 2368)[0]
        enabled = raw_trigger[2372:2399]
        if any(value not in (0, 1) for value in enabled):
            raise ValueError(f'TRIG trigger {trigger_index} has invalid enabled-player flags')
        initial_cursor = raw_trigger[2399]
        triggers.append({'conditions': condition_row, 'actions': action_row,
                         'enabled_players': enabled, 'initial_cursor': initial_cursor,
                         'execution_flags': execution_flags})

    if (expected_active_conditions is not None and
            active_conditions != expected_active_conditions):
        raise ValueError(f'TRIG has {active_conditions} active conditions, expected {expected_active_conditions}')
    if (expected_active_actions is not None and
            active_actions != expected_active_actions):
        raise ValueError(f'TRIG has {active_actions} active actions, expected {expected_active_actions}')

    doodads = []
    for offset in range(0, len(doodad_payload), 10):
        raw = doodad_payload[offset:offset + 10]
        type_id, x, y, owner, unused, flags = struct.unpack('<HHHBBH', raw)
        doodads.append({'type': type_id, 'x': x, 'y': y, 'owner': owner,
                        'unused': unused, 'flags': flags, 'raw': raw})

    properties = []
    for offset in range(0, len(payloads['UPRP']), 20):
        raw = payloads['UPRP'][offset:offset + 20]
        values = struct.unpack('<HHBBBBIHHI', raw)
        keys = ('valid_special', 'valid_fields', 'owner', 'hp_percent',
                'shield_percent', 'energy_percent', 'resources',
                'hangar_count', 'special', 'unused')
        properties.append(dict(zip(keys, values), raw=raw))
    setup = {'controllers': payloads['OWNR'], 'races': payloads['SIDE'],
             'force_by_slot': payloads['FORC'][:8], 'force_raw': payloads['FORC'],
             'property_used': payloads['UPUS']}

    return {'locations': locations, 'conditions': conditions, 'actions': actions,
            'triggers': triggers, 'doodads': doodads,
            'properties': properties, 'setup': setup,
            'active_condition_count': active_conditions,
            'active_action_count': active_actions,
            'source_record_sha256': {name: _sha256(data) for name, data in framed.items()},
            'source_chk_sha256': manifest['source_chk_sha256']}


def _u(value: int, width: int = 8) -> str:
    return f'0x{value:0{width}x}u'


def _bytes(data: bytes) -> str:
    return '{' + ','.join(_u(value, 2) for value in data) + '}'


def emit_header(data: dict) -> str:
    """Render the validated records as immutable, fixed-slot ROM arrays."""
    locations = data['locations']
    conditions = data['conditions']
    actions = data['actions']
    triggers = data['triggers']
    doodads = data['doodads']
    out = [
        '/* Private generated T4 numeric scenario data. Do not commit or distribute. */\n',
        '#ifndef SC64_T4_SCENARIO_GENERATED_H\n#define SC64_T4_SCENARIO_GENERATED_H\n',
        '#ifdef SC64_T4_RUNTIME\n#include "sc64_trigger_types.h"\n',
        f'static const Sc64T4Location sc64_t4_locations[{len(locations)}] = {{\n',
    ]
    for item in locations:
        values = ','.join([_u(item['left']), _u(item['top']), _u(item['right']),
                           _u(item['bottom']), _u(item['name_id'], 4),
                           _u(item['elevation'], 4), _bytes(item['raw'])])
        out.append('    {' + values + '},\n')
    out.append('};\n')
    out.append(f'static const Sc64T4UnitProperties sc64_t4_properties[{len(data["properties"])}] = {{\n')
    for row in data['properties']:
        values = [_u(row[key]) for key in (
            'valid_special', 'valid_fields', 'owner', 'hp_percent',
            'shield_percent', 'energy_percent', 'resources', 'hangar_count',
            'special', 'unused')]
        out.append('    {' + ','.join(values + [_bytes(row['raw'])]) + '},\n')
    out.append('};\n')
    out.append('static const Sc64T4SourceSetup sc64_t4_source_setup = {\n')
    out.append(',\n'.join('    ' + _bytes(data['setup'][key]) for key in
                         ('controllers', 'races', 'force_by_slot', 'force_raw', 'property_used')))
    out.append('\n};\n')
    out.append(f'static const Sc64T4Condition sc64_t4_conditions[{len(conditions)}] = {{\n')
    for row in conditions:
        values = [_u(row[key]) for key in ('location', 'player', 'amount')]
        values += [_u(row['unit'], 4)]
        values += [_u(row[key], 2) for key in ('comparison', 'opcode', 'resource', 'flags')]
        values += [_u(row['mask'], 4)]
        out.append('    {' + ','.join(values) + '},\n')
    out.append('};\n')
    out.append(f'static const Sc64T4Action sc64_t4_actions[{len(actions)}] = {{\n')
    for row in actions:
        values = [_u(row[key]) for key in ('location', 'string_id', 'wav_id', 'time',
                                            'player', 'argument')]
        values += [_u(row['unit'], 4)]
        values += [_u(row[key], 2) for key in ('opcode', 'modifier', 'flags')]
        values += [_bytes(row['trailing'])]
        out.append('    {' + ','.join(values) + '},\n')
    out.append('};\n')
    out.append(f'static const Sc64T4Doodad sc64_t4_doodads[{len(doodads)}] = {{\n')
    for row in doodads:
        values = [_u(row[key], 4) for key in ('type', 'x', 'y')]
        values += [_u(row['owner'], 2), _u(row['unused'], 2), _u(row['flags'], 4)]
        values += [_bytes(row['raw'])]
        out.append('    {' + ','.join(values) + '},\n')
    out.append('};\n')
    out.append(f'static const Sc64T4Trigger sc64_t4_triggers[{len(triggers)}] = {{\n')
    for index, trigger in enumerate(triggers):
        enabled = _bytes(trigger['enabled_players'])
        cond_ptr = f'&sc64_t4_conditions[{index * CONDITIONS_PER_TRIGGER}]'
        action_ptr = f'&sc64_t4_actions[{index * ACTIONS_PER_TRIGGER}]'
        out.append(f'    {{{cond_ptr},{action_ptr},{enabled},'
                   f'{_u(trigger["initial_cursor"], 2)},'
                   f'{_u(trigger["execution_flags"])}}},\n')
    out.append('};\n')
    out.append('static const Sc64T4Scenario sc64_t4_scenario = {\n'
               '    sc64_t4_locations, (u16)(sizeof(sc64_t4_locations)/sizeof(sc64_t4_locations[0])),\n'
               '    sc64_t4_triggers, (u16)(sizeof(sc64_t4_triggers)/sizeof(sc64_t4_triggers[0])),\n'
               '    sc64_t4_doodads, (u16)(sizeof(sc64_t4_doodads)/sizeof(sc64_t4_doodads[0]))\n'
               '};\n#endif /* SC64_T4_RUNTIME */\n#endif\n')
    return ''.join(out)


def pack(records_dir: Path = DEFAULT_RECORDS, manifest: Path = DEFAULT_MANIFEST,
         output: Path = DEFAULT_OUTPUT) -> dict:
    data = parse_source(records_dir, manifest)
    content = emit_header(data)
    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile('w', encoding='ascii', dir=output.parent,
                                     prefix=output.name + '.', delete=False) as stream:
        temp_path = Path(stream.name)
        stream.write(content)
        stream.flush()
    temp_path.replace(output)
    return {'header': str(output), 'header_sha256': _sha256(content.encode('ascii')),
            'locations': len(data['locations']), 'triggers': len(data['triggers']),
            'conditions': data['active_condition_count'],
            'actions': data['active_action_count'], 'doodads': len(data['doodads'])}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--records-dir', type=Path, default=DEFAULT_RECORDS)
    parser.add_argument('--manifest', type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument('--output', type=Path, default=DEFAULT_OUTPUT)
    args = parser.parse_args()
    try:
        report = pack(args.records_dir, args.manifest, args.output)
    except (OSError, ValueError, struct.error) as exc:
        parser.error(str(exc))
    print(json.dumps(report, indent=2))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
