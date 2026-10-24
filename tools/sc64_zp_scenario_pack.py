#!/usr/bin/env python3
"""Emit private numeric scenario metadata for authenticated Z5-Z10/P5-P10 CHKs.

This source-only packer validates complete staged CHKs against the pinned map
catalog and writes numeric trigger/setup tables under ignored .local storage.
It does not execute triggers or enable a campaign profile. Strings and WAV
payloads are authenticated as part of the CHK but are never emitted.
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
DEFAULT_SOURCE_ROOT = ROOT / '.local/sc64/zp-stage/generated/source_maps'
DEFAULT_OUTPUT_ROOT = ROOT / '.local/sc64/zp-stage/generated/scenarios'
ROM_SHA1 = 'a36d91b5aac7bb95df0d7de5f9814f5270e25c2b'
EXTRACTOR_COMMIT = '09b57f787a247107d1de82b8c5eab2734667ea1d'
TRIGGER_BYTES = 2400
ACTION_BYTES = 32
CONDITION_BYTES = 20
ACTION_OPCODE_LIMIT = 59
CONDITION_OPCODE_LIMIT = 24
REQUIRED_SETUP = (b'DIM ', b'MRGN', b'OWNR', b'SIDE', b'FORC',
                  b'UPRP', b'UPUS', b'PUNI', b'TRIG')
FIXED_SIZES = {b'DIM ': 4, b'OWNR': 12, b'SIDE': 12, b'FORC': 20,
               b'UPRP': 64 * 20, b'UPUS': 64, b'PUNI': 5700}
MISSIONS = {**{f'Z{i}': (15 + (i - 4), f'008/{0x17 + i - 4:03X}')
               for i in range(5, 11)},
            **{f'P{i}': (23 + (i - 4), f'008/{0x21 + i - 4:03X}')
               for i in range(5, 11)}}


def _load_catalog():
    path = ROOT / 'tools/sc64_races_pack.py'
    spec = importlib.util.spec_from_file_location('sc64_zp_scenario_catalog', path)
    if spec is None or spec.loader is None:
        raise RuntimeError('cannot load pinned race source catalog')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def parse_chk_sections(chk: bytes) -> dict[bytes, bytes]:
    """Parse a complete CHK section stream, rejecting duplicate/truncated data."""
    sections: dict[bytes, bytes] = {}
    offset = 0
    while offset < len(chk):
        if len(chk) - offset < 8:
            raise ValueError('source CHK has a truncated section header')
        tag = chk[offset:offset + 4]
        size = struct.unpack_from('<I', chk, offset + 4)[0]
        end = offset + 8 + size
        if end > len(chk):
            raise ValueError(f'source CHK section {tag!r} exceeds file bounds')
        if tag in sections:
            raise ValueError(f'source CHK has duplicate section {tag!r}')
        sections[tag] = chk[offset + 8:end]
        offset = end
    return sections


def _verify_manifest(manifest: dict, chk: bytes, sections: dict[bytes, bytes],
                     mission: str, expected_path: str, expected_hash: str) -> None:
    if not isinstance(manifest, dict):
        raise ValueError(f'{mission} source manifest is not an object')
    digest = hashlib.sha256(chk).hexdigest()
    expected_fields = {
        'schema': 1,
        'mission_id': mission,
        'source_map_path': expected_path,
        'source_rom_sha1': ROM_SHA1,
        'source_chk_bytes': len(chk),
        'source_chk_sha256': expected_hash,
        'source_chk_file': 'source.chk',
        'extractor_commit': EXTRACTOR_COMMIT,
    }
    for key, expected in expected_fields.items():
        if manifest.get(key) != expected:
            raise ValueError(f'{mission} source manifest {key} differs from pinned provenance')
    if digest != expected_hash:
        raise ValueError(f'{mission} source CHK hash differs from pinned catalog')
    manifest_sections = manifest.get('sections')
    if not isinstance(manifest_sections, list):
        raise ValueError(f'{mission} manifest lacks its complete section audit')
    seen: set[bytes] = set()
    for row in manifest_sections:
        if not isinstance(row, dict) or not isinstance(row.get('tag'), str):
            raise ValueError(f'{mission} manifest has a malformed section row')
        try:
            tag = row['tag'].encode('ascii')
        except UnicodeEncodeError as exc:
            raise ValueError(f'{mission} manifest contains a non-ASCII section tag') from exc
        if len(tag) != 4 or tag in seen or tag not in sections:
            raise ValueError(f'{mission} manifest has a duplicate or unknown section')
        seen.add(tag)
        payload = sections[tag]
        framed = tag + struct.pack('<I', len(payload)) + payload
        if (row.get('payload_bytes') != len(payload) or
                row.get('record_bytes') != len(framed) or
                row.get('record_sha256') != hashlib.sha256(framed).hexdigest()):
            raise ValueError(f'{mission} manifest section {tag!r} does not match CHK bytes')
    if seen != set(sections):
        raise ValueError(f'{mission} manifest does not cover every CHK section')


def _verify_framed_records(source_dir: Path, sections: dict[bytes, bytes]) -> None:
    records_dir = source_dir / 'records'
    if not records_dir.exists():
        return
    if not records_dir.is_dir():
        raise ValueError('records path is not a directory')
    seen: set[bytes] = set()
    for path in sorted(records_dir.glob('*.bin')):
        try:
            data = path.read_bytes()
        except OSError as exc:
            raise ValueError(f'cannot read framed source record {path.name}') from exc
        if len(data) < 8:
            raise ValueError(f'{path.name}: truncated framed record')
        tag = data[:4]
        length = struct.unpack_from('<I', data, 4)[0]
        payload = data[8:]
        if length != len(payload) or tag not in sections or sections[tag] != payload:
            raise ValueError(f'{path.name}: framed record differs from authenticated CHK section')
        if tag in seen:
            raise ValueError(f'{path.name}: duplicate framed section')
        expected_name = tag.decode('ascii').rstrip().replace(' ', '') + '.bin'
        if path.name != expected_name:
            raise ValueError(f'{path.name}: filename does not match framed section tag')
        seen.add(tag)


def _parse_actions_and_conditions(trigger_data: bytes, trigger_count: int,
                                  location_count: int) -> tuple[list[dict], list[dict], list[dict]]:
    actions: list[dict] = []
    conditions: list[dict] = []
    triggers: list[dict] = []
    for ti in range(trigger_count):
        base = ti * TRIGGER_BYTES
        trigger = trigger_data[base:base + TRIGGER_BYTES]
        first_condition, first_action = len(conditions), len(actions)
        inactive = False
        for ci in range(16):
            raw = trigger[ci * CONDITION_BYTES:(ci + 1) * CONDITION_BYTES]
            (location, player, amount, unit, comparison, opcode,
             resource, flags, mask) = struct.unpack('<IIIHBBBBH', raw)
            if opcode == 0:
                inactive = True
                if any(raw):
                    raise ValueError(f'TRIG {ti} condition {ci} has fields with opcode zero')
                continue
            if inactive:
                raise ValueError(f'TRIG {ti} has a noncontiguous active condition row')
            if opcode >= CONDITION_OPCODE_LIMIT:
                raise ValueError(f'TRIG {ti} condition {ci} opcode is outside pinned bounds')
            if location > location_count:
                raise ValueError(f'TRIG {ti} condition {ci} references an invalid location')
            conditions.append({'trigger': ti, 'index': ci, 'location': location,
                               'player': player, 'amount': amount, 'unit': unit,
                               'comparison': comparison, 'opcode': opcode,
                               'resource': resource, 'flags': flags, 'mask': mask})

        inactive = False
        for ai in range(64):
            start = 320 + ai * ACTION_BYTES
            raw = trigger[start:start + ACTION_BYTES]
            (location, string_id, wav_id, time, group1, group2, unit,
             opcode, modifier, flags, trailing) = struct.unpack('<IIIIIIHBBB3s', raw)
            if opcode == 0:
                inactive = True
                if any(raw):
                    raise ValueError(f'TRIG {ti} action {ai} has fields with opcode zero')
                continue
            if inactive:
                raise ValueError(f'TRIG {ti} has a noncontiguous active action row')
            if opcode >= ACTION_OPCODE_LIMIT:
                raise ValueError(f'TRIG {ti} action {ai} opcode is outside pinned bounds')
            if location > location_count:
                raise ValueError(f'TRIG {ti} action {ai} references an invalid location')
            actions.append({'trigger': ti, 'index': ai, 'location': location,
                            'string_id': string_id, 'wav_id': wav_id, 'time': time,
                            'group1': group1, 'group2': group2, 'unit': unit,
                            'opcode': opcode, 'modifier': modifier, 'flags': flags,
                            'trailing': list(trailing)})

        execution_flags = struct.unpack_from('<I', trigger, 2368)[0]
        enabled = list(trigger[2372:2399])
        cursor = trigger[2399]
        if any(value not in (0, 1) for value in enabled):
            raise ValueError(f'TRIG {ti} has invalid enabled-player flags')
        if cursor >= 64:
            raise ValueError(f'TRIG {ti} initial action cursor is outside its record')
        triggers.append({'index': ti, 'first_action': first_action,
                         'action_count': len(actions) - first_action,
                         'first_condition': first_condition,
                         'condition_count': len(conditions) - first_condition,
                         'execution_flags': execution_flags,
                         'initial_cursor': cursor, 'enabled_players': enabled})
    for trigger in triggers:
        a0, a1 = trigger['first_action'], trigger['first_action'] + trigger['action_count']
        c0, c1 = trigger['first_condition'], trigger['first_condition'] + trigger['condition_count']
        if not (0 <= a0 <= a1 <= len(actions) and 0 <= c0 <= c1 <= len(conditions)):
            raise ValueError(f"TRIG {trigger['index']} row slice is out of bounds")
        if any(row['trigger'] != trigger['index'] or row['index'] >= 64
               for row in actions[a0:a1]):
            raise ValueError(f"TRIG {trigger['index']} action slice does not match source")
        if any(row['trigger'] != trigger['index'] or row['index'] >= 16
               for row in conditions[c0:c1]):
            raise ValueError(f"TRIG {trigger['index']} condition slice does not match source")
    return triggers, actions, conditions


def parse_source(source_root: Path, mission: str) -> dict:
    mission = mission.upper()
    if mission not in MISSIONS:
        raise ValueError('mission must be one of Z5-Z10 or P5-P10')
    catalog = _load_catalog()
    expected_global_id, expected_path = MISSIONS[mission]
    catalog_rows = [(index, row) for index, row in enumerate(catalog.MISSIONS)
                    if row[0] == mission]
    if len(catalog_rows) != 1:
        raise ValueError(f'{mission} is not uniquely pinned in the source catalog')
    map_index, (_, map_path, _) = catalog_rows[0]
    global_id = catalog.global_id_for_map(map_index)
    if global_id != expected_global_id or map_path != expected_path:
        raise ValueError(f'{mission} stable catalog identity differs from audited IDs')
    expected_hash = catalog.RACE_EXTRA_CHK_SHA256.get(map_path)
    if expected_hash is None:
        raise ValueError(f'{mission} has no pinned CHK hash in the campaign source catalog')
    source_dir = source_root / mission.lower()
    try:
        manifest = json.loads((source_dir / 'manifest.json').read_text(encoding='utf-8'))
        chk = (source_dir / 'source.chk').read_bytes()
    except (OSError, json.JSONDecodeError) as exc:
        raise ValueError(f'cannot read staged {mission} source CHK and manifest') from exc
    sections = parse_chk_sections(chk)
    _verify_manifest(manifest, chk, sections, mission, map_path, expected_hash)
    _verify_framed_records(source_dir, sections)
    missing = [tag.decode('ascii').strip() for tag in REQUIRED_SETUP if tag not in sections]
    if missing:
        raise ValueError(f'{mission} CHK lacks required numeric scenario sections: {missing}')
    for tag, expected in FIXED_SIZES.items():
        if len(sections[tag]) != expected:
            raise ValueError(f'{mission} {tag.decode("ascii").strip()} section size is invalid')
    if len(sections[b'MRGN']) == 0 or len(sections[b'MRGN']) % 20 or len(sections[b'MRGN']) // 20 > 255:
        raise ValueError(f'{mission} MRGN section has an invalid location table length')
    if len(sections[b'TRIG']) == 0 or len(sections[b'TRIG']) % TRIGGER_BYTES:
        raise ValueError(f'{mission} TRIG section has an invalid record length')
    if len(sections[b'TRIG']) // TRIGGER_BYTES > 255:
        raise ValueError(f'{mission} trigger count exceeds numeric index bounds')
    width, height = struct.unpack('<HH', sections[b'DIM '])
    if width == 0 or height == 0 or width > 128 or height > 128:
        raise ValueError(f'{mission} map dimensions are outside imported-map bounds')

    locations = []
    for index in range(len(sections[b'MRGN']) // 20):
        left, top, right, bottom, name_id, flags = struct.unpack_from(
            '<IIIIHH', sections[b'MRGN'], index * 20)
        locations.append({'index': index + 1, 'left': left, 'top': top,
                          'right': right, 'bottom': bottom,
                          'name_id': name_id, 'flags': flags})
    trigger_count = len(sections[b'TRIG']) // TRIGGER_BYTES
    triggers, actions, conditions = _parse_actions_and_conditions(
        sections[b'TRIG'], trigger_count, len(locations))
    properties = [list(sections[b'UPRP'][i:i + 20])
                  for i in range(0, len(sections[b'UPRP']), 20)]
    return {
        'mission': mission, 'global_catalog_id': global_id,
        'source_map_path': map_path, 'source_chk_sha256': expected_hash,
        'width': width, 'height': height, 'trigger_count': trigger_count,
        'triggers': triggers, 'actions': actions, 'conditions': conditions,
        'locations': locations,
        'owners': list(sections[b'OWNR']), 'sides': list(sections[b'SIDE']),
        'forces': list(sections[b'FORC']), 'properties': properties,
        'property_used': list(sections[b'UPUS']), 'puni': list(sections[b'PUNI']),
        'section_sizes': {tag.decode('ascii').rstrip(): len(payload)
                          for tag, payload in sections.items()},
    }


def _u(value: int, width: int = 8) -> str:
    return f'0x{value:0{width}x}u'


def emit_header(data: dict) -> str:
    prefix = data['mission'].lower()
    guard = f'SC64_{data["mission"]}_SOURCE_SCENARIO_GENERATED_H'
    out = [f'#ifndef {guard}\n#define {guard}\n',
           f'#define SC64_{data["mission"]}_SOURCE_GLOBAL_ID {data["global_catalog_id"]}u\n',
           f'#define SC64_{data["mission"]}_SOURCE_MAP_COLS {data["width"]}u\n',
           f'#define SC64_{data["mission"]}_SOURCE_MAP_ROWS {data["height"]}u\n',
           f'#define SC64_{data["mission"]}_SOURCE_TRIGGER_COUNT {data["trigger_count"]}u\n',
           f'#define SC64_{data["mission"]}_SOURCE_ACTION_COUNT {len(data["actions"])}u\n',
           f'#define SC64_{data["mission"]}_SOURCE_CONDITION_COUNT {len(data["conditions"])}u\n',
           f'#define SC64_{data["mission"]}_SOURCE_LOCATION_COUNT {len(data["locations"])}u\n',
           f'typedef struct {{ unsigned int location, string_id, wav_id, time, group1, group2; unsigned short unit; unsigned char trigger,index,opcode,modifier,flags,trailing[3]; }} Sc64{data["mission"]}SourceAction;\n',
           f'typedef struct {{ unsigned int location,player,amount; unsigned short unit,mask; unsigned char trigger,index,comparison,opcode,resource,flags; }} Sc64{data["mission"]}SourceCondition;\n',
           f'typedef struct {{ unsigned short first_action,action_count,first_condition,condition_count; unsigned int execution_flags; unsigned char initial_cursor,enabled_players[27]; }} Sc64{data["mission"]}SourceTrigger;\n',
           f'typedef struct {{ unsigned int left,top,right,bottom; unsigned short name_id,flags; }} Sc64{data["mission"]}SourceLocation;\n',
           f'static const Sc64{data["mission"]}SourceAction sc64_{prefix}_source_actions[{len(data["actions"])}] = {{\n']
    for row in data['actions']:
        values = [_u(row[k]) for k in ('location','string_id','wav_id','time','group1','group2')]
        values += [_u(row['unit'],4)]
        values += [_u(row[k],2) for k in ('trigger','index','opcode','modifier','flags')]
        values += ['{' + ','.join(_u(v,2) for v in row['trailing']) + '}']
        out.append('    {' + ','.join(values) + '},\n')
    out.append('};\n')
    out.append(f'static const Sc64{data["mission"]}SourceCondition sc64_{prefix}_source_conditions[{len(data["conditions"])}] = {{\n')
    for row in data['conditions']:
        values = [_u(row[k]) for k in ('location','player','amount')]
        values += [_u(row['unit'],4), _u(row['mask'],4)]
        values += [_u(row[k],2) for k in ('trigger','index','comparison','opcode','resource','flags')]
        out.append('    {' + ','.join(values) + '},\n')
    out.append('};\n')
    out.append(f'static const Sc64{data["mission"]}SourceTrigger sc64_{prefix}_source_triggers[{len(data["triggers"])}] = {{\n')
    for row in data['triggers']:
        enabled = '{' + ','.join(_u(v,2) for v in row['enabled_players']) + '}'
        vals = [_u(row[k],4) for k in ('first_action','action_count','first_condition','condition_count')]
        vals += [_u(row['execution_flags']), _u(row['initial_cursor'],2), enabled]
        out.append('    {' + ','.join(vals) + '},\n')
    out.append('};\n')
    out.append(f'static const Sc64{data["mission"]}SourceLocation sc64_{prefix}_source_locations[{len(data["locations"])}] = {{\n')
    for row in data['locations']:
        vals = [_u(row[k]) for k in ('left','top','right','bottom')]
        vals += [_u(row['name_id'],4), _u(row['flags'],4)]
        out.append('    {' + ','.join(vals) + '},\n')
    out.append('};\n')
    for name in ('owners','sides','forces','property_used'):
        values = data[name]
        out.append(f'static const unsigned char sc64_{prefix}_source_{name}[{len(values)}] = {{')
        out.append(','.join(_u(v,2) for v in values) + '};\n')
    for name, rows in (('properties',data['properties']),):
        out.append(f'static const unsigned char sc64_{prefix}_source_{name}[{len(rows)}][20] = {{\n')
        for row in rows:
            out.append('    {' + ','.join(_u(v,2) for v in row) + '},\n')
        out.append('};\n')
    out.append(f'static const unsigned char sc64_{prefix}_source_puni[{len(data["puni"])}] = {{\n')
    for start in range(0, len(data['puni']), 24):
        out.append('    ' + ','.join(_u(v,2) for v in data['puni'][start:start + 24]) + ',\n')
    out.append('};\n#endif\n')
    return ''.join(out)


def count_create_unit_actions(actions: list[dict]) -> int:
    """Count CHK CreateUnitwithProperties (11) and CreateUnit (44) rows.

    Opcode 43 is SetInvincibility; treating it as a spawn inflates the source
    census and can mislead actor coverage audits.
    """
    return sum(row['opcode'] in (11, 44) for row in actions)


def verify_staged(source_root: Path, mission: str) -> dict:
    data = parse_source(source_root, mission)
    return {'mission': data['mission'], 'global_catalog_id': data['global_catalog_id'],
            'source_map_path': data['source_map_path'],
            'source_chk_sha256': data['source_chk_sha256'],
            'dimensions': [data['width'], data['height']],
            'trigger_records': data['trigger_count'], 'actions': len(data['actions']),
            'conditions': len(data['conditions']),
            'preserve_trigger_actions': sum(r['opcode'] == 3 for r in data['actions']),
            'create_unit_actions': count_create_unit_actions(data['actions']),
            'locations': len(data['locations'])}


def generate(source_root: Path, mission: str, output: Path) -> dict:
    data = parse_source(source_root, mission)
    content = emit_header(data)
    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile('w', encoding='ascii', dir=output.parent,
                                     prefix=output.name + '.', delete=False) as f:
        temp = Path(f.name)
        f.write(content)
    temp.replace(output)
    return verify_staged(source_root, mission)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-root', type=Path, default=DEFAULT_SOURCE_ROOT)
    parser.add_argument('--mission', required=True, choices=tuple(MISSIONS))
    parser.add_argument('--output', type=Path)
    parser.add_argument('--verify-staged', action='store_true')
    args = parser.parse_args()
    if args.verify_staged and args.output is not None:
        parser.error('--verify-staged cannot be combined with --output')
    if not args.verify_staged and args.output is None:
        args.output = DEFAULT_OUTPUT_ROOT / f'sc64_{args.mission.lower()}_scenario_generated.h'
    try:
        result = (verify_staged(args.source_root, args.mission) if args.verify_staged
                  else generate(args.source_root, args.mission, args.output))
    except (OSError, ValueError, RuntimeError) as exc:
        parser.error(str(exc))
    print(json.dumps(result, sort_keys=True))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
