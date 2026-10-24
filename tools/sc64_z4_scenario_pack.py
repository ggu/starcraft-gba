#!/usr/bin/env python3
"""Generate private typed Z4 trigger metadata from authenticated staged CHK records.

The source CHK, strings, WAV assets, and generated header remain under ignored
.local storage. This emits numeric dispatch metadata only; it is not a CHK
interpreter and does not infer action timing policy.
"""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
from pathlib import Path
import struct
import tempfile

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_SOURCE = ROOT / '.local/sc64/z4'
DEFAULT_OUTPUT = ROOT / '.local/sc64/z4/generated/sc64_z4_scenario_generated.h'
ACTION_COUNTS = (5, 14, 1, 24, 14, 3, 5)
TRIGGER_COUNT = 7
ACTIONS_PER_TRIGGER = 64
CONDITIONS_PER_TRIGGER = 16


def _load_pack():
    spec = importlib.util.spec_from_file_location('sc64_z4_pack_for_scenario', ROOT / 'tools/sc64_z4_pack.py')
    if spec is None or spec.loader is None:
        raise RuntimeError('cannot load Z4 source verifier')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def _parse_sections(chk: bytes) -> dict[bytes, bytes]:
    sections: dict[bytes, bytes] = {}
    offset = 0
    while offset < len(chk):
        if len(chk) - offset < 8:
            raise ValueError('source CHK has truncated section header')
        tag = chk[offset:offset + 4]
        size = struct.unpack_from('<I', chk, offset + 4)[0]
        end = offset + 8 + size
        if end > len(chk) or tag in sections:
            raise ValueError('source CHK has invalid or duplicate section')
        sections[tag] = chk[offset + 8:end]
        offset = end
    return sections


def _unframe(data: bytes, tag: bytes, size: int, label: str) -> bytes:
    if len(data) < 8 or data[:4] != tag or struct.unpack_from('<I', data, 4)[0] != len(data) - 8:
        raise ValueError(f'{label}: invalid framed record')
    payload = data[8:]
    if len(payload) != size:
        raise ValueError(f'{label}: unexpected payload size')
    return payload


def parse_source(source_dir: Path = DEFAULT_SOURCE) -> dict:
    """Authenticate staged source and decode typed numeric trigger metadata."""
    verifier = _load_pack()
    verifier.verify_staged(source_dir)
    chk = (source_dir / 'source.chk').read_bytes()
    sections = _parse_sections(chk)
    needed = (b'TRIG', b'MRGN', b'OWNR', b'SIDE', b'FORC', b'UPRP', b'UPUS')
    if any(tag not in sections for tag in needed):
        raise ValueError('authenticated source is missing a required runtime metadata section')
    trig = _unframe((source_dir / 'records/TRIG.bin').read_bytes(), b'TRIG', 7 * 2400, 'TRIG')
    mrgn = _unframe((source_dir / 'records/MRGN.bin').read_bytes(), b'MRGN', 64 * 20, 'MRGN')
    owners = _unframe((source_dir / 'records/OWNR.bin').read_bytes(), b'OWNR', 12, 'OWNR')
    sides = _unframe((source_dir / 'records/SIDE.bin').read_bytes(), b'SIDE', 12, 'SIDE')
    forces = _unframe((source_dir / 'records/FORC.bin').read_bytes(), b'FORC', 20, 'FORC')
    properties = _unframe((source_dir / 'records/UPRP.bin').read_bytes(), b'UPRP', 64 * 20, 'UPRP')
    prop_used = _unframe((source_dir / 'records/UPUS.bin').read_bytes(), b'UPUS', 64, 'UPUS')
    # Verify each framed source payload is the corresponding CHK section even
    # after verify_staged; this also keeps decoding tied directly to source.chk.
    for tag, payload in ((b'TRIG', trig), (b'MRGN', mrgn), (b'OWNR', owners),
                         (b'SIDE', sides), (b'FORC', forces), (b'UPRP', properties),
                         (b'UPUS', prop_used)):
        if sections.get(tag) != payload:
            raise ValueError(f'{tag.decode("ascii").strip()} differs from source CHK')
    locations = []
    for index in range(64):
        left, top, right, bottom, name_id, flags = struct.unpack_from('<IIIIHH', mrgn, index * 20)
        locations.append({'index': index + 1, 'left': left, 'top': top,
                          'right': right, 'bottom': bottom, 'name_id': name_id,
                          'flags': flags})
    actions: list[dict] = []
    conditions: list[dict] = []
    triggers: list[dict] = []
    for ti in range(TRIGGER_COUNT):
        base = ti * 2400
        trigger = trig[base:base + 2400]
        cond_rows = []
        for ci in range(CONDITIONS_PER_TRIGGER):
            raw = trigger[ci * 20:(ci + 1) * 20]
            loc, player, amount, unit, comparison, opcode, resource, flags, mask = struct.unpack('<IIIHBBBBH', raw)
            row = {'trigger': ti, 'index': ci, 'location': loc, 'player': player,
                   'amount': amount, 'unit': unit, 'comparison': comparison,
                   'opcode': opcode, 'resource': resource, 'flags': flags, 'mask': mask}
            cond_rows.append(row)
            if opcode:
                conditions.append(row)
        action_rows = []
        active_seen = 0
        inactive_seen = False
        for ai in range(ACTIONS_PER_TRIGGER):
            raw = trigger[320 + ai * 32:320 + (ai + 1) * 32]
            loc, string_id, wav_id, time, group1, group2, unit, opcode, modifier, flags, trailing = struct.unpack('<IIIIIIHBBB3s', raw)
            row = {'trigger': ti, 'index': ai, 'location': loc,
                   'string_id': string_id, 'wav_id': wav_id, 'time': time,
                   'group1': group1, 'group2': group2, 'unit': unit,
                   'opcode': opcode, 'modifier': modifier, 'flags': flags,
                   'trailing': list(trailing)}
            action_rows.append(row)
            if opcode:
                if inactive_seen:
                    raise ValueError(f'TRIG {ti} has noncontiguous active actions')
                actions.append(row)
                active_seen += 1
            else:
                inactive_seen = True
        if active_seen != ACTION_COUNTS[ti]:
            raise ValueError(f'TRIG {ti} active action count differs from audited Z4 model')
        enabled = list(trigger[2372:2399])
        if any(v not in (0, 1) for v in enabled):
            raise ValueError(f'TRIG {ti} has invalid enabled-player flags')
        triggers.append({'index': ti, 'first_action': sum(ACTION_COUNTS[:ti]),
                         'action_count': ACTION_COUNTS[ti], 'conditions': cond_rows,
                         'actions': action_rows, 'execution_flags': struct.unpack_from('<I', trigger, 2368)[0],
                         'enabled_players': enabled, 'initial_cursor': trigger[2399]})
    if len(actions) != sum(ACTION_COUNTS) or len(actions) != 66:
        raise ValueError('source action total does not match the 66-action Z4 model')
    if any(row['location'] > 64 for row in actions if row['location']):
        raise ValueError('action references a location outside source MRGN table')
    prop_rows = [list(properties[i:i + 20]) for i in range(0, len(properties), 20)]
    return {'source_chk_sha256': hashlib.sha256(chk).hexdigest(),
            'triggers': triggers, 'actions': actions, 'conditions': conditions,
            'locations': locations, 'owners': list(owners), 'sides': list(sides),
            'forces': list(forces), 'properties': prop_rows,
            'property_used': list(prop_used)}


def _u(v: int, width: int = 8) -> str:
    return f'0x{v:0{width}x}u'


def emit_header(data: dict) -> str:
    """Render compact raw numeric rows; no source text/WAV content is emitted."""
    out = ['/* Private generated Z4 numeric trigger dispatch metadata. */\n',
           '#ifndef SC64_Z4_SCENARIO_GENERATED_H\n#define SC64_Z4_SCENARIO_GENERATED_H\n',
           f'#define SC64_Z4_SOURCE_CHK_SHA256_HEX \"{data["source_chk_sha256"]}\"\n',
           'typedef struct { unsigned int location,string_id,wav_id,time,group1,group2; unsigned short unit; unsigned char trigger,index,opcode,modifier,flags,trailing[3]; } Sc64Z4SourceAction;\n',
           'typedef struct { unsigned int location,player,amount; unsigned short unit,mask; unsigned char trigger,index,comparison,opcode,resource,flags; } Sc64Z4SourceCondition;\n',
           'typedef struct { unsigned short first_action,action_count; unsigned int execution_flags; unsigned char initial_cursor,enabled_players[27]; } Sc64Z4SourceTrigger;\n',
           f'static const Sc64Z4SourceAction sc64_z4_source_actions[{len(data["actions"])}] = {{\n']
    for r in data['actions']:
        vals = [_u(r[k]) for k in ('location','string_id','wav_id','time','group1','group2')]
        vals += [_u(r['unit'],4), _u(r['trigger'],2), _u(r['index'],2), _u(r['opcode'],2), _u(r['modifier'],2), _u(r['flags'],2)]
        vals += ['{' + ','.join(_u(v,2) for v in r['trailing']) + '}']
        out.append('    {' + ','.join(vals) + '},\n')
    out.append('};\n')
    out.append(f'static const Sc64Z4SourceCondition sc64_z4_source_conditions[{len(data["conditions"])}] = {{\n')
    for r in data['conditions']:
        vals = [_u(r[k]) for k in ('location','player','amount')]
        vals += [_u(r['unit'],4), _u(r['mask'],4)]
        vals += [_u(r[k],2) for k in ('trigger','index','comparison','opcode','resource','flags')]
        out.append('    {' + ','.join(vals) + '},\n')
    out.append('};\n')
    out.append(f'static const Sc64Z4SourceTrigger sc64_z4_source_triggers[{len(data["triggers"])}] = {{\n')
    for r in data['triggers']:
        enabled = '{' + ','.join(_u(v,2) for v in r['enabled_players']) + '}'
        out.append(f'    {{{_u(r["first_action"],4)},{_u(r["action_count"],4)},{_u(r["execution_flags"])},{_u(r["initial_cursor"],2)},{enabled}}},\n')
    out.append('};\n')
    for name in ('owners','sides','forces','property_used'):
        values = data[name]
        out.append(f'static const unsigned char sc64_z4_source_{name}[{len(values)}] = {{')
        out.append(','.join(_u(v,2) for v in values))
        out.append('};\n')
    out.append(f'static const unsigned char sc64_z4_source_properties[{len(data["properties"])}][20] = {{\n')
    for row in data['properties']:
        out.append('    {' + ','.join(_u(v,2) for v in row) + '},\n')
    out.append('};\n')
    out.append('typedef struct { unsigned int left,top,right,bottom; unsigned short name_id,flags; } Sc64Z4SourceLocation;\n')
    out.append(f'static const Sc64Z4SourceLocation sc64_z4_source_locations[{len(data["locations"])}] = {{\n')
    for r in data['locations']:
        out.append('    {' + ','.join([_u(r[k]) for k in ('left','top','right','bottom')] + [_u(r['name_id'],4),_u(r['flags'],4)]) + '},\n')
    out.append('};\n#endif\n')
    return ''.join(out)


def generate(source_dir: Path = DEFAULT_SOURCE, output: Path = DEFAULT_OUTPUT) -> dict:
    data = parse_source(source_dir)
    content = emit_header(data)
    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile('w', encoding='ascii', dir=output.parent,
                                     prefix=output.name + '.', delete=False) as f:
        temp = Path(f.name)
        f.write(content)
    temp.replace(output)
    return {'output': str(output), 'source_chk_sha256': data['source_chk_sha256'],
            'actions': len(data['actions']), 'conditions': len(data['conditions']),
            'triggers': len(data['triggers']),
            'header_sha256': hashlib.sha256(content.encode('ascii')).hexdigest()}


def main(argv=None) -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--source', type=Path, default=DEFAULT_SOURCE)
    p.add_argument('--output', type=Path, default=DEFAULT_OUTPUT)
    args = p.parse_args(argv)
    try:
        print(generate(args.source, args.output))
    except (OSError, ValueError, struct.error, RuntimeError) as e:
        p.error(str(e))
    return 0

if __name__ == '__main__':
    raise SystemExit(main())
