#!/usr/bin/env python3
"""Write a private metadata/trigger audit for base Terran campaign maps.

Only summaries and parsed mission logic are written, beneath ignored .local/.
The verified ROM and extractor are local inputs; no game data is shipped.
"""
from __future__ import annotations

import argparse
from collections import Counter
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_ROM = ROOT / '.local/sc64/rom/StarCraft64-USA.z64'
DEFAULT_OUT = ROOT / '.local/sc64/full-campaign-audit.json'
ROM_SHA1 = 'a36d91b5aac7bb95df0d7de5f9814f5270e25c2b'
EXTRACTOR_COMMIT = '09b57f787a247107d1de82b8c5eab2734667ea1d'
CAMPAIGN_ENTRY = re.compile(r'^008/0([0-9A-Fa-f]{2})$')
BUILDINGS = set(range(106, 176))
MINERALS = {176, 177, 178}
GEYSERS = {188}
READINESS = {path: 'reduced GBA mission implementation' for path in
             ('008/00A', '008/00B', '008/014', '008/015', '008/01E', '008/01F')}
SELECTOR_LABELS = {229: 'Any Unit', 230: 'Men', 231: 'Buildings', 232: 'Factories'}
KNOWN_UNIT_LABELS = {0: 'Terran Marine', 7: 'Terran SCV', 37: 'Zerg Zergling',
                     84: 'Protoss Observer', 101: 'Special Map Revealer',
                     106: 'Terran Command Center',
                     109: 'Terran Supply Depot', 111: 'Terran Barracks',
                     131: 'Zerg Hatchery', 176: 'Mineral Field 1',
                     177: 'Mineral Field 2', 178: 'Mineral Field 3', 188: 'Vespene Geyser'}


def module_at(path: Path, name: str):
    spec = importlib.util.spec_from_file_location(name, path)
    if not spec or not spec.loader:
        raise RuntimeError(f'Cannot load audit helper: {path}')
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


def active_locations(payload: bytes, string_table: bytes, extractor) -> list[dict]:
    import struct
    return [{'index': index + 1, 'left': l, 'top': t, 'right': r, 'bottom': b,
             'string_id': name, 'name': extractor._chk_string(string_table, name) if name else ''}
            for index in range(len(payload) // 20)
            for l, t, r, b, name, flags in [struct.unpack_from('<IIIIHH', payload, index * 20)]
            if l or t or r or b or name]


def run(rom_path: Path, output: Path) -> dict:
    importer = module_at(ROOT / 'tools/sc64_import.py', 'sc64_audit_import')
    extractor = importer.upstream_module(importer.UPSTREAM)
    rom = extractor.load_rom(str(rom_path))
    digest = hashlib.sha1(rom).hexdigest()
    if digest != ROM_SHA1:
        raise ValueError(f'Expected verified USA ROM ({ROM_SHA1}), got {digest}')
    archive = extractor.BoltArchive(rom)
    maps = []
    for info, chk in extractor.find_maps(archive):
        match = CAMPAIGN_ENTRY.match(info.bolt_path)
        if not match or not (0x0A <= int(match.group(1), 16) <= 0x27):
            continue
        s = importer.sections(chk, extractor)
        units = importer.unit_rows(s.get(b'UNIT', b''))
        types = Counter(u['source_type'] for u in units if u['source_type'] != 214)
        triggers = importer.trigger_rows(s.get(b'TRIG', b''), s.get(b'STR ', b''), extractor)
        for trigger in triggers:
            for condition in trigger['conditions']:
                value = condition['unit_type']
                uses_unit = condition['name'] in ('Command', 'Bring', 'Kill')
                condition['unit_type_interpretation'] = (
                    SELECTOR_LABELS.get(value) or KNOWN_UNIT_LABELS.get(value) or
                    'raw unit ID; label not verified by this audit') if uses_unit else 'not used by condition'
                group = condition['player_group']
                condition['group_interpretation'] = (
                    f'Player {group + 1}' if 0 <= group < 12 else {
                        13: 'Current Player', 14: 'Foes', 15: 'Allies',
                        16: 'Neutral Players', 17: 'All Players',
                        18: 'Force 1', 19: 'Force 2', 20: 'Force 3', 21: 'Force 4'
                    }.get(group, 'raw group/player selector'))
            for action in trigger['actions']:
                value = action['unit_type']
                uses_unit = action['name'] in ('CreateUnitwithProperties', 'CreateUnit', 'KillUnit',
                                                 'RemoveUnit', 'ModifyUnitHitPoints', 'ModifyUnitEnergy',
                                                 'ModifyUnitShieldPoints', 'ModifyUnitResourceAmount',
                                                 'ModifyUnitHangerCount')
                action['unit_type_interpretation'] = (KNOWN_UNIT_LABELS.get(value) or
                    'raw unit ID; label not verified by this audit') if uses_unit else 'not used by action'
        owners = list(s.get(b'OWNR', b''))
        sides = list(s.get(b'SIDE', b''))
        forces = s.get(b'FORC', b'')
        owner_map = [{'slot': i, 'owner_raw': owner,
                      'side_raw': sides[i] if i < len(sides) else None,
                      'unit_count': sum(u['owner_raw'] == i for u in units if u['source_type'] != 214)}
                     for i, owner in enumerate(owners)]
        conds = Counter(c['name'] for trigger in triggers for c in trigger['conditions'])
        acts = Counter(a['name'] for trigger in triggers for a in trigger['actions'])
        objectives = [{'trigger': t['record'], 'action': a['name'], 'text': a.get('text', ''),
                       'seconds': a['time'] // 1000 if a.get('time') else None}
                      for t in triggers for a in t['actions']
                      if a['name'] in ('SetMissionObjectives', 'DisplayTextMessage')]
        dialogues = [{'trigger': t['record'], 'action': a['name'], 'string_id': a['string_id'],
                      'text': a.get('text', ''), 'location': a['location'], 'time_ms': a['time']}
                     for t in triggers for a in t['actions']
                     if a['name'] in ('Transmission', 'DisplayTextMessage')]
        maps.append({
            'path': info.bolt_path, 'title': info.name, 'dimensions': [info.width, info.height],
            'cells': info.width * info.height, 'tileset_id': info.tileset,
            'tileset': info.tileset_name,
            'players': info.players, 'owners_and_sides': owner_map,
            'forces_raw_hex': forces.hex(),
            'placements': {'raw_unit_records': len(units),
                           'start_location_markers': sum(u['source_type'] == 214 for u in units),
                           'units': sum(n for typ, n in types.items() if typ not in BUILDINGS | MINERALS | GEYSERS),
                           'buildings': sum(n for typ, n in types.items() if typ in BUILDINGS),
                           'mineral_fields': sum(n for typ, n in types.items() if typ in MINERALS),
                           'geysers': sum(n for typ, n in types.items() if typ in GEYSERS),
                           'by_source_type': {str(k): v for k, v in sorted(types.items())}},
            'locations': active_locations(s.get(b'MRGN', b''), s.get(b'STR ', b''), extractor),
            'trigger_records': len(triggers), 'condition_counts': dict(sorted(conds.items())),
            'action_counts': dict(sorted(acts.items())), 'triggers': triggers,
            'objectives_and_text': objectives, 'mission_text_read': dialogues,
            'engine_gaps': ['No general CHK trigger interpreter; mission-specific adapters only.',
                            'No full mission briefing/transmission playback or authored wait sequencing.',
                            'No generic trigger-created units with CHK unit properties.',
                            'No generic CHK AI script runner or location-bound AI script dispatch.'],
            'readiness': READINESS.get(info.bolt_path, 'unsupported: no authored GBA mission adapter'),
        })
    maps.sort(key=lambda m: m['path'])
    if len(maps) != 30:
        raise ValueError(f'Expected campaign entries 008/00A..008/027 (30), found {len(maps)}')
    readiness_counts = Counter(m['readiness'] for m in maps)
    result = {'schema': 1,
              'source': {'rom_filename': rom_path.name, 'normalized_rom_sha1': digest,
                         'extractor': 'references/sc64-maps/extract_sc64_maps.py',
                         'extractor_commit': EXTRACTOR_COMMIT},
              'scope': 'BOLT 008/00A through 008/027 inclusive; base Terran campaign scenario entries.',
              'count': len(maps), 'readiness_counts': dict(readiness_counts), 'maps': maps}
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2, ensure_ascii=True) + '\n', encoding='utf-8')
    return result


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rom', type=Path, default=DEFAULT_ROM)
    parser.add_argument('--out', type=Path, default=DEFAULT_OUT)
    args = parser.parse_args()
    if not args.rom.is_file():
        parser.error(f'ROM does not exist: {args.rom}')
    try:
        result = run(args.rom, args.out)
    except (OSError, ValueError, RuntimeError) as exc:
        print(f'sc64_campaign_audit: {exc}', file=sys.stderr)
        raise SystemExit(1)
    print(f"Audited {result['count']} base campaign maps: {args.out}")


if __name__ == '__main__':
    main()
