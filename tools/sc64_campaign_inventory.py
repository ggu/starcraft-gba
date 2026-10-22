#!/usr/bin/env python3
"""Privately inventory Terran campaign scenarios in the verified USA ROM.

The report contains metadata and opcode coverage, not scenario bytes. It is
written beneath ignored .local/ and never advertises unsupported missions as
playable GBA content.
"""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import sys
from collections import Counter

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_ROM = ROOT / '.local/sc64/rom/StarCraft64-USA.z64'
DEFAULT_OUT = ROOT / '.local/sc64/campaign-index.json'
EXPECTED_ROM_SHA1 = 'a36d91b5aac7bb95df0d7de5f9814f5270e25c2b'
UPSTREAM_COMMIT = '09b57f787a247107d1de82b8c5eab2734667ea1d'

# Brood War unit types grouped by faction, used only to describe the scenario
# roster. Human-side race in OWNR/SIDE is the primary Terran campaign filter.
TERRAN_TYPES = {0, 1, 2, 3, 5, 7, 8, 9, 11, 12, 32, 34,
                106, 107, 108, 109, 110, 111, 112, 113, 114, 115,
                116, 117, 118, 120, 122, 123, 124, 125, 126, 127}
ZERG_TYPES = set(range(35, 59)) | set(range(130, 151))
PROTOSS_TYPES = set(range(60, 106)) | set(range(154, 175))


def load_importer():
    path = ROOT / 'tools/sc64_import.py'
    spec = importlib.util.spec_from_file_location('sc64_campaign_import', path)
    if not spec or not spec.loader:
        raise RuntimeError(f'Cannot load local CHK audit helpers: {path}')
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def run(rom_path: Path, output: Path) -> dict:
    importer = load_importer()
    extractor = importer.upstream_module(importer.UPSTREAM)
    rom = extractor.load_rom(str(rom_path))
    sha1 = hashlib.sha1(rom).hexdigest()
    if sha1 != EXPECTED_ROM_SHA1:
        raise ValueError(f'Expected the verified USA ROM ({EXPECTED_ROM_SHA1}), got {sha1}')
    archive = extractor.BoltArchive(rom)
    scenarios = []
    terran_campaign_count = 0
    all_condition_counts = Counter()
    all_action_counts = Counter()
    for info, chk in extractor.find_maps(archive):
        sections = importer.sections(chk, extractor)
        owners = sections.get(b'OWNR', b'')
        sides = sections.get(b'SIDE', b'')
        human_terran_slots = [i for i, (owner, side) in enumerate(zip(owners, sides))
                              if owner == 6 and side == 1]
        if not human_terran_slots:
            continue

        units = importer.unit_rows(sections.get(b'UNIT', b''))
        types = Counter(row['source_type'] for row in units if row['source_type'] != 214)
        faction_totals = {
            'terran': sum(count for unit_type, count in types.items() if unit_type in TERRAN_TYPES),
            'zerg': sum(count for unit_type, count in types.items() if unit_type in ZERG_TYPES),
            'protoss': sum(count for unit_type, count in types.items() if unit_type in PROTOSS_TYPES),
        }
        triggers = importer.trigger_rows(sections.get(b'TRIG', b''),
                                         sections.get(b'STR ', b''), extractor)
        condition_counts = Counter(c['opcode'] for trigger in triggers for c in trigger['conditions'])
        action_counts = Counter(a['opcode'] for trigger in triggers for a in trigger['actions'])
        all_condition_counts.update(condition_counts)
        all_action_counts.update(action_counts)
        # The port contains authored mission-specific subsets rather than a
        # general trigger VM. Keep this list descriptive and never imply full coverage.
        implemented = info.bolt_path in ('008/00A', '008/00B')
        scenarios.append({
            'path': info.bolt_path,
            'title': info.name,
            'description': info.description,
            'dimensions': {'width': info.width, 'height': info.height},
            'tileset': info.tileset_name,
            'human_terran_slots': human_terran_slots,
            'unit_placements': len(units),
            'faction_unit_counts': faction_totals,
            'condition_opcode_counts': dict(sorted(condition_counts.items())),
            'action_opcode_counts': dict(sorted(action_counts.items())),
            'trigger_records': len(triggers),
            'readiness': 'playable reduced implementation' if implemented else 'unsupported in GBA port',
            'trigger_coverage': ('mission-specific selected rules; not a general CHK interpreter'
                                 if implemented else 'unsupported; no GBA mission adapter'),
        })
        terran_campaign_count += 1

    result = {
        'schema': 1,
        'source': {
            'rom_filename': rom_path.name,
            'normalized_rom_sha1': sha1,
            'extractor': 'references/sc64-maps/extract_sc64_maps.py',
            'extractor_commit': UPSTREAM_COMMIT,
        },
        'classification': 'Terran campaign means at least one human/user-select slot has Terran race in CHK SIDE.',
        'readiness_note': 'Only T1 Wasteland (008/00A) and T2 Backwater Station (008/00B) have reduced GBA mission adapters. Every other indexed Terran scenario is unsupported.',
        'terran_campaign_map_count': terran_campaign_count,
        'trigger_opcode_inventory': {
            'conditions': dict(sorted(all_condition_counts.items())),
            'actions': dict(sorted(all_action_counts.items())),
            'coverage_gap': 'The GBA port has mission-specific trigger rules and no general CHK trigger interpreter; opcode presence is inventory evidence, not a support claim.',
        },
        'maps': scenarios,
    }
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
        print(f'sc64_campaign_inventory: {exc}', file=sys.stderr)
        return
    playable = sum(x['readiness'].startswith('playable') for x in result['maps'])
    print(f"Indexed {result['terran_campaign_map_count']} Terran campaign maps ({playable} playable reduced implementations): {args.out}")


if __name__ == '__main__':
    main()
