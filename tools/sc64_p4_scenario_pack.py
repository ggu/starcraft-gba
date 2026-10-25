#!/usr/bin/env python3
"""Emit private numeric dispatch metadata for the authenticated P4 triggers.

The CHK and source records stay under ignored .local storage. Generated output
contains complete numeric action/setup tables, not source dialogue or assets;
it does not implement trigger execution or mark P4 playable.
"""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import tempfile

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_SOURCE = ROOT / '.local/sc64/p4'
DEFAULT_OUTPUT = ROOT / '.local/sc64/p4/generated/sc64_p4_scenario_generated.h'
DEFAULT_REPORT = ROOT / '.local/sc64/p4/generated/scenario-audit.json'
TRIGGER_COUNT = 13
ACTIONS_PER_TRIGGER = 64
CONDITIONS_PER_TRIGGER = 16
ACTION_COUNTS = (1, 6, 6, 16, 8, 25, 9, 45, 6, 2, 21, 3, 5)
CONDITION_COUNTS = (1, 2, 1, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1)
CHK_SHA256 = '40f30ccedf87a3b9c88308dc3092611ceff10185ddaf32471104902042a93ecd'
PTEC_BYTES = 12 * 76
UNIT_BYTES = 201 * 36
P4_HUMAN_SLOT = 7
PSIONIC_STORM_TECH_ID = 19
PTEC_TECHS_PER_PLAYER = 24
PTEC_PLAYER_TECH_BYTES = 12 * PTEC_TECHS_PER_PLAYER
PTEC_GLOBAL_TECH_BYTES = 24
PTEC_PLAYER_BYTES = PTEC_TECHS_PER_PLAYER
PTEC_AVAILABLE_OFFSET = 0
PTEC_RESEARCHED_OFFSET = PTEC_PLAYER_TECH_BYTES
PTEC_GLOBAL_AVAILABLE_OFFSET = 2 * PTEC_PLAYER_TECH_BYTES
PTEC_GLOBAL_RESEARCHED_OFFSET = PTEC_GLOBAL_AVAILABLE_OFFSET + PTEC_GLOBAL_TECH_BYTES
PTEC_USE_DEFAULT_OFFSET = PTEC_GLOBAL_RESEARCHED_OFFSET + PTEC_GLOBAL_TECH_BYTES
UNIT_ID_OFFSET = 8
UNIT_STATE_FLAGS_OFFSET = 14
UNIT_ENERGY_VALID = 0x0008
UNIT_ENERGY_PERCENT_OFFSET = 19
P4_CASTER_RAW_TYPES = (67, 79)


def _load_pack():
    spec = importlib.util.spec_from_file_location(
        'sc64_p4_pack_for_scenario', ROOT / 'tools/sc64_p4_pack.py')
    if spec is None or spec.loader is None:
        raise RuntimeError('cannot load P4 staged-source verifier')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def _parse_sections(chk: bytes) -> dict[bytes, bytes]:
    sections: dict[bytes, bytes] = {}
    offset = 0
    while offset < len(chk):
        if len(chk) - offset < 8:
            raise ValueError('P4 source CHK has a truncated section header')
        tag = chk[offset:offset + 4]
        size = struct.unpack_from('<I', chk, offset + 4)[0]
        end = offset + 8 + size
        if end > len(chk) or tag in sections:
            raise ValueError('P4 source CHK has an invalid or duplicate section')
        sections[tag] = chk[offset + 8:end]
        offset = end
    return sections


def _unframe(data: bytes, tag: bytes, size: int, label: str) -> bytes:
    if (len(data) < 8 or data[:4] != tag or
            struct.unpack_from('<I', data, 4)[0] != len(data) - 8):
        raise ValueError(f'{label}: invalid framed source record')
    payload = data[8:]
    if len(payload) != size:
        raise ValueError(f'{label}: unexpected source payload size')
    return payload


def create_instance_count(action: dict) -> int:
    """Apply the audited native opcode-11 rule: modifier zero means one."""
    if action.get('opcode') != 11:
        raise ValueError('expected Create Unit with Properties opcode 11')
    raw = action.get('modifier')
    if not isinstance(raw, int) or not 0 <= raw <= 255:
        raise ValueError('create-unit quantity operand is outside its byte field')
    return raw or 1


def parse_source(source_dir: Path = DEFAULT_SOURCE) -> dict:
    """Verify and decode every P4 trigger plus its numeric setup tables."""
    verifier = _load_pack()
    verifier.verify_staged(source_dir)
    try:
        chk = (source_dir / 'source.chk').read_bytes()
        if hashlib.sha256(chk).hexdigest() != CHK_SHA256:
            raise ValueError('P4 scenario CHK hash differs from the pinned source')
        sections = _parse_sections(chk)
        required = (b'TRIG', b'MRGN', b'OWNR', b'SIDE', b'FORC',
                    b'UPRP', b'UPUS', b'PTEC', b'UNIT')
        if any(tag not in sections for tag in required):
            raise ValueError('authenticated P4 CHK is missing a scenario metadata section')
        framed_records = {
            b'TRIG': (b'TRIG', TRIGGER_COUNT * 2400, 'TRIG'),
            b'MRGN': (b'MRGN', 64 * 20, 'MRGN'),
            b'OWNR': (b'OWNR', 12, 'OWNR'),
            b'SIDE': (b'SIDE', 12, 'SIDE'),
            b'FORC': (b'FORC', 20, 'FORC'),
            b'UPRP': (b'UPRP', 64 * 20, 'UPRP'),
            b'UPUS': (b'UPUS', 64, 'UPUS'),
        }
        records = {}
        for tag, (record_tag, size, label) in framed_records.items():
            payload = _unframe((source_dir / 'records' /
                                f'{label}.bin').read_bytes(), record_tag, size, label)
            if sections[tag] != payload:
                raise ValueError(f'{label} differs from the authenticated source CHK')
            records[tag] = payload
    except OSError as exc:
        raise ValueError(f'cannot read verified P4 source metadata: {exc}') from exc

    trigger_data = records[b'TRIG']
    actions: list[dict] = []
    conditions: list[dict] = []
    triggers: list[dict] = []
    first_action = 0
    first_condition = 0
    for ti in range(TRIGGER_COUNT):
        base = ti * 2400
        trigger = trigger_data[base:base + 2400]
        condition_rows = []
        inactive_condition_seen = False
        for ci in range(CONDITIONS_PER_TRIGGER):
            raw = trigger[ci * 20:(ci + 1) * 20]
            (location, player, amount, unit, comparison, opcode,
             resource, flags, mask) = struct.unpack('<IIIHBBBBH', raw)
            row = {'trigger': ti, 'index': ci, 'location': location,
                   'player': player, 'amount': amount, 'unit': unit,
                   'comparison': comparison, 'opcode': opcode,
                   'resource': resource, 'flags': flags, 'mask': mask}
            if opcode:
                if inactive_condition_seen:
                    raise ValueError(f'TRIG {ti} has noncontiguous active conditions')
                if location > 64:
                    raise ValueError(f'TRIG {ti} condition {ci} references an invalid location')
                condition_rows.append(row)
                conditions.append(row)
            else:
                inactive_condition_seen = True
                if any(raw):
                    raise ValueError(f'TRIG {ti} has nonzero fields in an inactive condition')

        action_rows = []
        inactive_action_seen = False
        for ai in range(ACTIONS_PER_TRIGGER):
            raw = trigger[320 + ai * 32:320 + (ai + 1) * 32]
            (location, string_id, wav_id, time, group1, group2, unit,
             opcode, modifier, flags, trailing) = struct.unpack('<IIIIIIHBBB3s', raw)
            row = {'trigger': ti, 'index': ai, 'location': location,
                   'string_id': string_id, 'wav_id': wav_id, 'time': time,
                   'group1': group1, 'group2': group2, 'unit': unit,
                   'opcode': opcode, 'modifier': modifier, 'flags': flags,
                   'trailing': list(trailing)}
            if opcode:
                if inactive_action_seen:
                    raise ValueError(f'TRIG {ti} has noncontiguous active actions')
                if location > 64:
                    raise ValueError(f'TRIG {ti} action {ai} references an invalid location')
                action_rows.append(row)
                actions.append(row)
            else:
                inactive_action_seen = True
                if any(raw):
                    raise ValueError(f'TRIG {ti} has nonzero fields in an inactive action')

        if len(action_rows) != ACTION_COUNTS[ti]:
            raise ValueError(f'TRIG {ti} active action count differs from audited P4 source')
        if len(condition_rows) != CONDITION_COUNTS[ti]:
            raise ValueError(f'TRIG {ti} active condition count differs from audited P4 source')
        execution_flags = struct.unpack_from('<I', trigger, 2368)[0]
        enabled = list(trigger[2372:2399])
        cursor = trigger[2399]
        if any(value not in (0, 1) for value in enabled):
            raise ValueError(f'TRIG {ti} has invalid enabled-player flags')
        if cursor >= ACTIONS_PER_TRIGGER:
            raise ValueError(f'TRIG {ti} initial action cursor is outside its 64-action record')
        triggers.append({'index': ti, 'first_action': first_action,
                         'action_count': len(action_rows),
                         'first_condition': first_condition,
                         'condition_count': len(condition_rows),
                         'execution_flags': execution_flags,
                         'initial_cursor': cursor, 'enabled_players': enabled})
        first_action += len(action_rows)
        first_condition += len(condition_rows)

    if len(triggers) != TRIGGER_COUNT or len(actions) != sum(ACTION_COUNTS):
        raise ValueError('P4 source does not contain all thirteen audited trigger records')
    if len(conditions) != sum(CONDITION_COUNTS):
        raise ValueError('P4 source condition total differs from the audited source')
    for trigger in triggers:
        start, end = trigger['first_action'], trigger['first_action'] + trigger['action_count']
        if start < 0 or end > len(actions) or end < start:
            raise ValueError(f"TRIG {trigger['index']} action slice is out of bounds")
        if any(row['trigger'] != trigger['index'] or row['index'] >= ACTIONS_PER_TRIGGER
               for row in actions[start:end]):
            raise ValueError(f"TRIG {trigger['index']} action slice does not match its source")
        cstart = trigger['first_condition']
        cend = cstart + trigger['condition_count']
        if cstart < 0 or cend > len(conditions) or cend < cstart:
            raise ValueError(f"TRIG {trigger['index']} condition slice is out of bounds")
        if any(row['trigger'] != trigger['index'] or row['index'] >= CONDITIONS_PER_TRIGGER
               for row in conditions[cstart:cend]):
            raise ValueError(f"TRIG {trigger['index']} condition slice does not match its source")

    raw_properties = records[b'UPRP']
    property_rows = [list(raw_properties[i:i + 20])
                     for i in range(0, len(raw_properties), 20)]
    create_actions = []
    by_trigger: dict[str, dict[str, int]] = {}
    by_type: dict[str, dict[str, int]] = {}
    for action in actions:
        if action['opcode'] != 11:
            continue
        count = create_instance_count(action)
        raw_type = str(action['unit'])
        trigger_key = str(action['trigger'])
        by_trigger.setdefault(trigger_key, {})[raw_type] = (
            by_trigger.setdefault(trigger_key, {}).get(raw_type, 0) + count)
        by_type.setdefault(raw_type, {})[trigger_key] = (
            by_type.setdefault(raw_type, {}).get(trigger_key, 0) + count)
        create_actions.append({'trigger': action['trigger'], 'index': action['index'],
                               'raw_unit': action['unit'],
                               'raw_count_modifier': action['modifier'],
                               'instances_per_execution': count,
                               'group1': action['group1'], 'group2': action['group2'],
                               'flags': action['flags']})
    ptec = sections[b'PTEC']
    unit = sections[b'UNIT']
    if len(ptec) != PTEC_BYTES or len(unit) != UNIT_BYTES:
        raise ValueError('PTEC or UNIT source section has an unexpected size')
    player_tech = P4_HUMAN_SLOT * PTEC_PLAYER_BYTES + PSIONIC_STORM_TECH_ID
    global_tech = PSIONIC_STORM_TECH_ID
    storm_available_value = ptec[PTEC_AVAILABLE_OFFSET + player_tech]
    storm_researched_value = ptec[PTEC_RESEARCHED_OFFSET + player_tech]
    storm_global_available_value = ptec[PTEC_GLOBAL_AVAILABLE_OFFSET + global_tech]
    storm_global_researched_value = ptec[PTEC_GLOBAL_RESEARCHED_OFFSET + global_tech]
    storm_use_default_value = ptec[PTEC_USE_DEFAULT_OFFSET + player_tech]
    ptec_values = (storm_available_value, storm_researched_value,
                   storm_global_available_value, storm_global_researched_value,
                   storm_use_default_value)
    if any(value not in (0, 1) for value in ptec_values):
        raise ValueError('P4 slot 7 has invalid Psionic Storm PTEC boolean flags')
    storm_effective_available = (storm_global_available_value if storm_use_default_value
                                 else storm_available_value)
    storm_effective_researched = (storm_global_researched_value if storm_use_default_value
                                  else storm_researched_value)
    caster_energy_percent: dict[str, int] = {}
    for offset in range(0, len(unit), 36):
        raw_type = struct.unpack_from('<H', unit, offset + UNIT_ID_OFFSET)[0]
        if raw_type not in P4_CASTER_RAW_TYPES:
            continue
        state_flags = struct.unpack_from('<H', unit, offset + UNIT_STATE_FLAGS_OFFSET)[0]
        if not state_flags & UNIT_ENERGY_VALID:
            raise ValueError(f'P4 caster raw {raw_type} lacks the UNIT energy-valid flag')
        percent = unit[offset + UNIT_ENERGY_PERCENT_OFFSET]
        if percent > 100:
            raise ValueError(f'P4 caster raw {raw_type} has invalid source energy percent')
        key = str(raw_type)
        if key in caster_energy_percent and caster_energy_percent[key] != percent:
            raise ValueError(f'P4 caster raw {raw_type} has inconsistent source energy')
        caster_energy_percent[key] = percent
    if set(caster_energy_percent) != {str(raw) for raw in P4_CASTER_RAW_TYPES}:
        raise ValueError('P4 UNIT source is missing a High Templar or Tassadar energy row')
    if storm_effective_researched != 1:
        raise ValueError('P4 slot 7 does not effectively mark Psionic Storm researched in PTEC')
    if storm_effective_available != 1:
        raise ValueError('P4 slot 7 does not effectively enable Psionic Storm in PTEC')
    if any(value != 100 for value in caster_energy_percent.values()):
        raise ValueError('P4 source caster energy is not the audited 100 percent')
    creation = {'actions': create_actions,
                'instances_per_execution_by_trigger_and_raw_unit': by_trigger,
                'instances_per_execution_by_raw_unit_and_trigger': by_type,
                'total_instances_per_execution': sum(
                    item['instances_per_execution'] for item in create_actions),
                'quantity_rule': 'opcode 11 modifier zero normalizes to one, per audited native helper'}
    return {'source_chk_sha256': hashlib.sha256(chk).hexdigest(),
            'triggers': triggers, 'actions': actions, 'conditions': conditions,
            'locations': [
                {'index': i + 1, 'left': struct.unpack_from('<I', records[b'MRGN'], i * 20)[0],
                 'top': struct.unpack_from('<I', records[b'MRGN'], i * 20 + 4)[0],
                 'right': struct.unpack_from('<I', records[b'MRGN'], i * 20 + 8)[0],
                 'bottom': struct.unpack_from('<I', records[b'MRGN'], i * 20 + 12)[0],
                 'name_id': struct.unpack_from('<H', records[b'MRGN'], i * 20 + 16)[0],
                 'flags': struct.unpack_from('<H', records[b'MRGN'], i * 20 + 18)[0]}
                for i in range(64)],
            'owners': list(records[b'OWNR']), 'sides': list(records[b'SIDE']),
            'forces': list(records[b'FORC']), 'properties': property_rows,
            'property_used': list(records[b'UPUS']), 'actor_creation': creation,
            'psionic_storm': {'slot': P4_HUMAN_SLOT,
                              'tech_id': PSIONIC_STORM_TECH_ID,
                              'availability_ptec': storm_available_value,
                              'researched_ptec': storm_researched_value,
                              'global_availability_ptec': storm_global_available_value,
                              'global_researched_ptec': storm_global_researched_value,
                              'use_default_ptec': storm_use_default_value,
                              'effective_available': storm_effective_available,
                              'effective_researched': storm_effective_researched,
                              'energy_percent_by_raw_type': caster_energy_percent}}


def verify_staged(source_dir: Path = DEFAULT_SOURCE) -> dict:
    """Reverify source provenance and all scenario dispatch constraints."""
    data = parse_source(source_dir)
    return {'source_chk_sha256': data['source_chk_sha256'],
            'triggers': len(data['triggers']), 'actions': len(data['actions']),
            'conditions': len(data['conditions']),
            'create_unit_actions': len(data['actor_creation']['actions']),
            'created_instances_per_execution':
                data['actor_creation']['total_instances_per_execution']}


def _u(value: int, width: int = 8) -> str:
    return f'0x{value:0{width}x}u'


def emit_header(data: dict) -> str:
    """Emit numeric P4 source fields only; never copy STR text or WAV bytes."""
    out = ['/* Private generated P4 numeric trigger dispatch metadata. */\n',
           '#ifndef SC64_P4_SCENARIO_GENERATED_H\n#define SC64_P4_SCENARIO_GENERATED_H\n',
           f'#define SC64_P4_SOURCE_CHK_SHA256_HEX "{data["source_chk_sha256"]}"\n',
           'typedef struct { unsigned int location,string_id,wav_id,time,group1,group2; unsigned short unit; unsigned char trigger,index,opcode,modifier,flags,trailing[3]; } Sc64P4SourceAction;\n',
           'typedef struct { unsigned int location,player,amount; unsigned short unit,mask; unsigned char trigger,index,comparison,opcode,resource,flags; } Sc64P4SourceCondition;\n',
           'typedef struct { unsigned short first_action,action_count,first_condition,condition_count; unsigned int execution_flags; unsigned char initial_cursor,enabled_players[27]; } Sc64P4SourceTrigger;\n',
           f'static const Sc64P4SourceAction sc64_p4_source_actions[{len(data["actions"])}] = {{\n']
    for row in data['actions']:
        values = [_u(row[key]) for key in
                  ('location', 'string_id', 'wav_id', 'time', 'group1', 'group2')]
        values += [_u(row['unit'], 4)]
        values += [_u(row[key], 2) for key in
                   ('trigger', 'index', 'opcode', 'modifier', 'flags')]
        values += ['{' + ','.join(_u(value, 2) for value in row['trailing']) + '}']
        out.append('    {' + ','.join(values) + '},\n')
    out.append('};\n')
    storm = data['psionic_storm']
    out.append('static const unsigned char sc64_p4_source_storm_slot7_available_ptec = ' +
               _u(storm['availability_ptec'], 2) + ';\n')
    out.append('static const unsigned char sc64_p4_source_storm_slot7_researched_ptec = ' +
               _u(storm['researched_ptec'], 2) + ';\n')
    out.append('static const unsigned char sc64_p4_source_storm_global_default_available = ' +
               _u(storm['global_availability_ptec'], 2) + ';\n')
    out.append('static const unsigned char sc64_p4_source_storm_global_default_researched = ' +
               _u(storm['global_researched_ptec'], 2) + ';\n')
    out.append('static const unsigned char sc64_p4_source_storm_use_default = ' +
               _u(storm['use_default_ptec'], 2) + ';\n')
    out.append('static const unsigned char sc64_p4_source_storm_effective_available = ' +
               _u(storm['effective_available'], 2) + ';\n')
    out.append('static const unsigned char sc64_p4_source_storm_effective_researched = ' +
               _u(storm['effective_researched'], 2) + ';\n')
    for raw in P4_CASTER_RAW_TYPES:
        out.append(f'static const unsigned char sc64_p4_source_energy_percent_{raw} = ' +
                   _u(storm['energy_percent_by_raw_type'][str(raw)], 2) + ';\n')
    out.append(f'static const Sc64P4SourceCondition sc64_p4_source_conditions[{len(data["conditions"])}] = {{\n')
    for row in data['conditions']:
        values = [_u(row[key]) for key in ('location', 'player', 'amount')]
        values += [_u(row['unit'], 4), _u(row['mask'], 4)]
        values += [_u(row[key], 2) for key in
                   ('trigger', 'index', 'comparison', 'opcode', 'resource', 'flags')]
        out.append('    {' + ','.join(values) + '},\n')
    out.append('};\n')
    out.append(f'static const Sc64P4SourceTrigger sc64_p4_source_triggers[{len(data["triggers"])}] = {{\n')
    for row in data['triggers']:
        enabled = '{' + ','.join(_u(value, 2) for value in row['enabled_players']) + '}'
        out.append('    {' + ','.join(_u(row[key], 4) for key in
                    ('first_action', 'action_count', 'first_condition', 'condition_count')) +
                    f',{_u(row["execution_flags"])},{_u(row["initial_cursor"], 2)},{enabled}' + '},\n')
    out.append('};\n')
    for key in ('owners', 'sides', 'forces', 'property_used'):
        values = data[key]
        out.append(f'static const unsigned char sc64_p4_source_{key}[{len(values)}] = {{')
        out.append(','.join(_u(value, 2) for value in values))
        out.append('};\n')
    out.append(f'static const unsigned char sc64_p4_source_properties[{len(data["properties"])}][20] = {{\n')
    for row in data['properties']:
        out.append('    {' + ','.join(_u(value, 2) for value in row) + '},\n')
    out.append('};\n')
    out.append('typedef struct { unsigned int left,top,right,bottom; unsigned short name_id,flags; } Sc64P4SourceLocation;\n')
    out.append(f'static const Sc64P4SourceLocation sc64_p4_source_locations[{len(data["locations"])}] = {{\n')
    for row in data['locations']:
        values = [_u(row[key]) for key in ('left', 'top', 'right', 'bottom')]
        values += [_u(row['name_id'], 4), _u(row['flags'], 4)]
        out.append('    {' + ','.join(values) + '},\n')
    out.append('};\n#endif\n')
    return ''.join(out)


def generate(source_dir: Path = DEFAULT_SOURCE,
             output: Path = DEFAULT_OUTPUT,
             report: Path = DEFAULT_REPORT) -> dict:
    data = parse_source(source_dir)
    content = emit_header(data)
    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile('w', encoding='ascii', dir=output.parent,
                                     prefix=output.name + '.', delete=False) as stream:
        temp = Path(stream.name)
        stream.write(content)
    temp.replace(output)
    report.parent.mkdir(parents=True, exist_ok=True)
    report.write_text(json.dumps({'schema': 1, 'source_chk_sha256': data['source_chk_sha256'],
                                  'triggers': len(data['triggers']),
                                  'actions': len(data['actions']),
                                  'conditions': len(data['conditions']),
                                  'actor_creation': data['actor_creation']},
                                 indent=2, sort_keys=True) + '\n', encoding='ascii')
    return {'output': str(output), 'report': str(report),
            'source_chk_sha256': data['source_chk_sha256'],
            'triggers': len(data['triggers']), 'actions': len(data['actions']),
            'conditions': len(data['conditions']),
            'created_instances_per_execution':
                data['actor_creation']['total_instances_per_execution'],
            'header_sha256': hashlib.sha256(content.encode('ascii')).hexdigest()}


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, default=DEFAULT_SOURCE)
    parser.add_argument('--output', type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument('--report', type=Path, default=DEFAULT_REPORT)
    parser.add_argument('--verify-staged', action='store_true',
                        help='verify authenticated P4 scenario records without generating output')
    args = parser.parse_args(argv)
    try:
        result = (verify_staged(args.source) if args.verify_staged else
                  generate(args.source, args.output, args.report))
    except (OSError, ValueError, RuntimeError, struct.error, KeyError) as exc:
        parser.error(str(exc))
    print(json.dumps(result, sort_keys=True))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
