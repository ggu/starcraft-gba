#!/usr/bin/env python3
"""Build a private numeric source audit for Terran campaign missions 7–10.

Reads only the already verified local ROM via the pinned extractor. The
generated report is deliberately confined to ignored .local/ output and omits
original dialogue, payload bytes, and asset data.
"""
from __future__ import annotations

import argparse
from collections import Counter
import hashlib
import importlib.util
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_ROM = ROOT / '.local/sc64/rom/StarCraft64-USA.z64'
DEFAULT_OUT = ROOT / '.local/sc64/terran-late/terran-late-audit.json'
ROM_SHA1 = 'a36d91b5aac7bb95df0d7de5f9814f5270e25c2b'
EXTRACTOR_COMMIT = '09b57f787a247107d1de82b8c5eab2734667ea1d'
N64_COMPARISON_MEANINGS = {
    # Confirmed from the exact-ROM native helper 0x800862F0. It compares the
    # measured count (a1) with the condition amount (a2).
    0: 'at_least',
    1: 'at_most',
    10: 'exactly',
}


def n64_comparison_meaning(value: int) -> str | None:
    """Return only comparison values verified in the native count helper."""
    return N64_COMPARISON_MEANINGS.get(value)


def n64_selector26_matches(current: int, candidate: int,
                           current_to_candidate: int,
                           candidate_to_current: int) -> bool:
    """Exact slot predicate observed in the native selector-26 path.

    The native path excludes the current slot and excludes a candidate only
    when both directional relation bytes equal 2. This preserves the raw
    relation-code fact without assigning a higher-level diplomacy label.
    """
    if not 0 <= current < 8 or not 0 <= candidate < 8:
        raise ValueError('native player slots must be in [0, 8)')
    return candidate != current and not (
        current_to_candidate == 2 and candidate_to_current == 2)


def _native_condition_notes(condition: dict) -> dict:
    comparison = condition['comparison']
    selector = condition['selector']
    result = {'native_comparison': n64_comparison_meaning(comparison)}
    if selector == 26:
        result['native_selector_rule'] = (
            'candidate slot differs from current slot and is not a mutual '
            'relation-code-2 pair')
    else:
        result['native_selector_rule'] = None
    return result
PATHS = ('008/010', '008/011', '008/012', '008/013')
BUILDING_IDS = set(range(106, 176)) | {195}
MINERAL_IDS = {176, 177, 178}
GEYSER_IDS = {188}


def global_id(path: str) -> int:
    """Campaign IDs are contiguous from 008/006 -> ID 0."""
    if path not in PATHS and path != '008/00F':
        raise ValueError(f'not a supported late-campaign BOLT path: {path}')
    return int(path[-3:], 16) - 6


def create_instance_count(action: dict) -> int:
    """Return native opcode-11 quantity (modifier zero normalizes to one)."""
    if action.get('opcode') != 11:
        raise ValueError('expected CreateUnitWithProperties opcode 11')
    count = action.get('modifier')
    if not isinstance(count, int) or count < 0 or count > 255:
        raise ValueError('create count operand is outside its byte field')
    return count or 1


def placement_counts(type_counts: Counter, start_count: int) -> dict:
    """Separate CHK type-band classification from the runtime actor pools."""
    return {
        'ordinary_units_source_class': sum(n for t, n in type_counts.items()
                                             if t not in BUILDING_IDS | MINERAL_IDS | GEYSER_IDS),
        'buildings_runtime_class': sum(n for t, n in type_counts.items() if t in BUILDING_IDS),
        'minerals': sum(n for t, n in type_counts.items() if t in MINERAL_IDS),
        'geysers': sum(n for t, n in type_counts.items() if t in GEYSER_IDS),
        'start_markers': start_count,
        'runtime_units_if_raw195_is_building': sum(n for t, n in type_counts.items()
            if t not in set(range(106, 176)) | {195} | MINERAL_IDS | GEYSER_IDS),
    }


def load_module(path: Path, name: str):
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f'cannot load local helper {path}')
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


def active_locations(payload: bytes) -> list[dict]:
    import struct
    if len(payload) % 20 or len(payload) > 255 * 20:
        raise ValueError(f'MRGN has unexpected size {len(payload)}')
    out = []
    for i in range(len(payload) // 20):
        left, top, right, bottom, string_id, flags = struct.unpack_from(
            '<IIIIHH', payload, i * 20)
        if left or top or right or bottom or string_id:
            out.append({'index': i + 1, 'rect': [left, top, right, bottom],
                        'name_string_id': string_id, 'flags': flags})
    return out


def numeric_trigger_rows(payload: bytes) -> list[dict]:
    """Parse numeric trigger fields while intentionally excluding text bytes."""
    import struct
    if len(payload) % 2400:
        raise ValueError('TRIG payload is not a whole number of records')
    rows = []
    for index in range(len(payload) // 2400):
        base = index * 2400
        conditions, actions = [], []
        for slot in range(16):
            raw = payload[base + slot * 20:base + (slot + 1) * 20]
            if not any(raw):
                continue
            location, selector, amount = struct.unpack_from('<III', raw)
            unit = struct.unpack_from('<H', raw, 12)[0]
            comparison, opcode, resource, flags = raw[14:18]
            mask = struct.unpack_from('<H', raw, 18)[0]
            conditions.append({'slot': slot, 'opcode': opcode,
                'location': location, 'selector': selector, 'amount': amount,
                'unit': unit, 'comparison': comparison, 'resource': resource,
                'flags': flags, 'mask': mask,
                **_native_condition_notes({'comparison': comparison,
                                          'selector': selector})})
        action_base = base + 320
        for slot in range(64):
            raw = payload[action_base + slot * 32:action_base + (slot + 1) * 32]
            if not any(raw):
                continue
            location, _string_id, wav_id, time, group1, group2 = struct.unpack_from(
                '<IIIIII', raw)
            unit = struct.unpack_from('<H', raw, 24)[0]
            opcode, modifier, flags = raw[26:29]
            trailing = raw[29:32]
            actions.append({'slot': slot, 'opcode': opcode, 'location': location,
                'wav': wav_id, 'time_ms': time, 'group1': group1,
                'group2': group2, 'unit': unit, 'modifier': modifier,
                'flags': flags, 'trailing_nonzero': any(trailing)})
        flags = struct.unpack_from('<I', payload, base + 2368)[0]
        enabled = list(payload[base + 2372:base + 2399])
        cursor = payload[base + 2399]
        if conditions or actions or flags or any(enabled):
            rows.append({'record': index, 'conditions': conditions,
                         'actions': actions, 'execution_flags': flags,
                         'enabled_selector_indices': [i for i, v in enumerate(enabled) if v],
                         'initial_action_cursor': cursor})
    return rows


def parse_chk(chk: bytes, extractor, importer) -> tuple[dict[bytes, bytes], dict]:
    sections = importer.sections(chk, extractor)
    required = (b'DIM ', b'ERA ', b'MTXM', b'UNIT', b'OWNR', b'SIDE',
                b'FORC', b'MRGN', b'TRIG', b'THG2', b'DD2 ', b'UPRP', b'UPUS')
    missing = [tag.decode('ascii').strip() for tag in required if tag not in sections]
    if missing:
        raise ValueError(f'CHK is missing sections: {missing}')
    return sections, {}


def audit_one(path: str, info, chk: bytes, extractor, importer) -> dict:
    sections, _ = parse_chk(chk, extractor, importer)
    units = importer.unit_rows(sections[b'UNIT'])
    starts = [u for u in units if u['source_type'] == 214]
    source = [u for u in units if u['source_type'] != 214]
    type_counts = Counter(u['source_type'] for u in source)
    categories = placement_counts(type_counts, len(starts))
    triggers = numeric_trigger_rows(sections[b'TRIG'])
    condition_counts = Counter(c['opcode'] for tr in triggers for c in tr['conditions'])
    action_counts = Counter(a['opcode'] for tr in triggers for a in tr['actions'])
    owner_counts = Counter(u['owner_raw'] for u in source)
    type_to_owner = {}
    for row in source:
        type_to_owner.setdefault(str(row['source_type']), Counter())[str(row['owner_raw'])] += 1
    props = [sections[b'UPRP'][i:i + 20] for i in range(0, len(sections[b'UPRP']), 20)]
    if len(sections[b'UPRP']) % 20:
        raise ValueError(f'{path} UPRP framing invalid')
    upus = sections[b'UPUS']
    if len(upus) != 64:
        raise ValueError(f'{path} UPUS has {len(upus)} bytes, expected 64')
    used = [i + 1 for i, value in enumerate(upus) if value]
    create_refs = [a['group2'] for tr in triggers for a in tr['actions'] if a['opcode'] == 11]
    create_actions = []
    for tr in triggers:
        preserves = any(a['opcode'] == 3 for a in tr['actions'])
        for action in tr['actions']:
            if action['opcode'] == 11:
                create_actions.append({
                    'trigger': tr['record'], 'action_slot': action['slot'],
                    'raw_unit': action['unit'],
                    # CHK group1 is the source owner/selector. Opcode 11's
                    # native count field is modifier, with zero normalized to
                    # a single created actor by the audited N64 handler.
                    'source_count': create_instance_count(action),
                    'property_reference_1based': action['group2'] if action['flags'] & 8 else None,
                    'location': action['location'], 'trigger_preserved': preserves,
                    'enabled_selector_indices': tr['enabled_selector_indices'],
                })
    terminal_rows = []
    for tr in triggers:
        for action in tr['actions']:
            if action['opcode'] in (1, 2):
                terminal_rows.append({'trigger': tr['record'],
                    'terminal_opcode': action['opcode'],
                    'action_flags': action['flags'],
                    'action_disabled': bool(action['flags'] & 2),
                    'enabled_selector_indices': tr['enabled_selector_indices'],
                    'execution_flags': tr['execution_flags'],
                    'conditions': tr['conditions']})
    return {
        'path': path,
        'title': info.name,
        'global_mission_id': int(path[-3:], 16) - 6,
        'dimensions': [info.width, info.height],
        'tileset_id': info.tileset,
        'tileset': info.tileset_name,
        'source_chk_sha256': hashlib.sha256(chk).hexdigest(),
        'section_sizes_sha256': {tag.decode('ascii').strip(): {
            'size': len(payload), 'sha256': hashlib.sha256(payload).hexdigest()}
            for tag, payload in sorted(sections.items())},
        'placement_categories': categories,
        'source_type_frequencies': {str(t): type_counts[t] for t in sorted(type_counts)},
        'owner_slot_counts': {str(o): owner_counts[o] for o in sorted(owner_counts)},
        'type_by_source_owner': {t: dict(sorted(c.items(), key=lambda x: int(x[0])))
                                 for t, c in sorted(type_to_owner.items(), key=lambda x: int(x[0]))},
        'trigger_record_count': len(triggers),
        'condition_opcode_counts': dict(sorted(condition_counts.items())),
        'action_opcode_counts': dict(sorted(action_counts.items())),
        'triggers_numeric': triggers,
        'locations_numeric': active_locations(sections[b'MRGN']),
        'doodad_counts_by_type': {str(t): n for t, n in sorted(Counter(
            int.from_bytes(sections[b'THG2'][i:i + 2], 'little')
            for i in range(0, len(sections[b'THG2']), 10)).items())},
        'dd2_sprite_count': len(sections[b'DD2 ']) // 8,
        'thg2_count': len(sections[b'THG2']) // 10,
        'properties_count': len(props),
        'used_property_indices_1based': used,
        'create_property_refs_1based': create_refs,
        'create_action_instances': create_actions,
        'create_units_first_pass': sum(a['source_count'] for a in create_actions),
        'preserved_create_units_per_repeat': sum(a['source_count'] for a in create_actions
                                                   if a['trigger_preserved']),
        'terminal_predicates': terminal_rows,
        'create_reference_missing_or_unmarked': sorted(
            set(create_refs) - set(used)),
        'owner_raw': list(sections[b'OWNR']),
        'side_raw': list(sections[b'SIDE']),
        'force_raw': list(sections[b'FORC']),
        'source_behavior_notes': [
            'Numeric-only action and condition operands; source strings/dialogue omitted.',
            'Action timing fields are raw milliseconds; any runtime conversion is a separate policy.',
            'Source CHK classification is preserved separately from compact runtime actor-pool classification.',
            'Native comparator helper 0x800862F0 confirms raw values 0, 1, and 10 as at-least, at-most, and exactly; other values remain uninterpreted.',
            'Native selector path 0x80087E54 -> 0x800873D4 for selector 26 excludes the current slot and excludes candidates only for a mutual relation-code-2 pair; no higher-level diplomacy label is inferred.'
        ]
    }


def run(rom_path: Path, output: Path) -> dict:
    importer = load_module(ROOT / 'tools/sc64_import.py', 'sc64_late_audit_import')
    extractor = importer.upstream_module(importer.UPSTREAM)
    rom = extractor.load_rom(str(rom_path))
    digest = hashlib.sha1(rom).hexdigest()
    if digest != ROM_SHA1:
        raise ValueError(f'expected verified ROM SHA-1 {ROM_SHA1}, got {digest}')
    archive = extractor.BoltArchive(rom)
    found = {}
    for info, chk in extractor.find_maps(archive):
        path = info.bolt_path
        if path in PATHS:
            if path in found:
                raise ValueError(f'duplicate exact map path {path}')
            found[path] = audit_one(path, info, chk, extractor, importer)
    if tuple(sorted(found)) != PATHS:
        raise ValueError(f'expected exact paths {PATHS}, found {tuple(sorted(found))}')
    result = {
        'schema': 1,
        'source': {'normalized_rom_sha1': digest,
                   'extractor': 'references/sc64-maps/extract_sc64_maps.py',
                   'extractor_commit': EXTRACTOR_COMMIT},
        'scope': 'Terran original campaign missions 7–10; exact BOLT paths 008/010–008/013.',
        'mission_count': 4,
        'missions': [found[path] for path in PATHS],
    }
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2, sort_keys=True) + '\n', encoding='utf-8')
    return result


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rom', type=Path, default=DEFAULT_ROM)
    parser.add_argument('--out', type=Path, default=DEFAULT_OUT)
    args = parser.parse_args()
    if not args.rom.is_file():
        parser.error(f'ROM does not exist: {args.rom}')
    result = run(args.rom, args.out)
    print(f"audited {result['mission_count']} exact maps; wrote private numeric summary")


if __name__ == '__main__':
    main()
