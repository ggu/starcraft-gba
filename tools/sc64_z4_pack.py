#!/usr/bin/env python3
"""Stage a private, source-only Zerg 4 map import from an owned SC64 ROM.

The tool verifies the known ROM, extractor revision and Z4 CHK hash, then
retains the complete CHK and authenticated framed records beneath ignored
.local/sc64/z4/. It does not create a runtime header or enable a campaign.
"""
from __future__ import annotations

import argparse
from collections import Counter
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_ROM = ROOT / '.local/sc64/rom/StarCraft64-USA.z64'
DEFAULT_OUT = ROOT / '.local/sc64/z4'
ROM_SHA1 = 'a36d91b5aac7bb95df0d7de5f9814f5270e25c2b'
EXTRACTOR_COMMIT = '09b57f787a247107d1de82b8c5eab2734667ea1d'
BOLT_PATH = '008/017'
MISSION_ID = 'Z4'
GLOBAL_CATALOG_INDEX = 15
CHK_SHA256 = '2cb8e516f07651801c1d6f519017d494af87c479a0ba8df9e33af34b14ec685a'
EXPECTED_DIMENSIONS = (96, 96)
EXPECTED_TILESET_ID = 3

# This exact source histogram makes additions, omissions, or previously
# unclassified UNIT rows a hard error. All rows (including start markers and
# rows with no current runtime mapping) are retained verbatim in source.chk.
EXPECTED_SOURCE_TYPES = {
    0: 27, 3: 12, 7: 4, 8: 5, 11: 3, 30: 3, 32: 2, 37: 6, 38: 4,
    41: 3, 42: 2, 45: 2, 106: 3, 109: 16, 110: 2, 111: 2, 112: 1,
    113: 2, 114: 3, 115: 3, 120: 2, 122: 1, 123: 1, 124: 21,
    131: 1, 143: 1, 144: 4, 146: 2, 149: 1, 150: 1, 176: 5,
    177: 20, 178: 14, 188: 2, 214: 3,
}

# Preserve source sections needed to inspect placements, terrain, ownership,
# locations, properties, switches and trigger behavior. source.chk separately
# preserves every section, including records not currently interpreted here.
RETAINED_SECTIONS = {
    b'DIM ': 'DIM.bin', b'ERA ': 'ERA.bin', b'MTXM': 'MTXM.bin',
    b'TILE': 'TILE.bin', b'UNIT': 'UNIT.bin', b'OWNR': 'OWNR.bin',
    b'SIDE': 'SIDE.bin', b'FORC': 'FORC.bin', b'MRGN': 'MRGN.bin',
    b'TRIG': 'TRIG.bin', b'STR ': 'STR.bin', b'SWNM': 'SWNM.bin',
    b'THG2': 'THG2.bin', b'PUNI': 'PUNI.bin', b'UPRP': 'UPRP.bin',
    b'UPUS': 'UPUS.bin',
}
EXPECTED_RECORD_COUNTS = {
    b'DIM ': 4, b'ERA ': 2, b'MTXM': 96 * 96 * 2, b'TILE': 96 * 96 * 2,
    b'UNIT': 184 * 36, b'OWNR': 12, b'SIDE': 12, b'FORC': 20,
    b'MRGN': 64 * 20, b'TRIG': 7 * 2400, b'SWNM': 1024,
    b'THG2': 11 * 10, b'PUNI': 5700, b'UPRP': 64 * 20,
    b'UPUS': 64,
}

BUILDING_TYPES = set(range(106, 176)) - {150}  # Mature Chrysalis uses the unit pool.
MINERAL_TYPES = {176, 177, 178}
GEYSER_TYPES = {188}
START_TYPE = 214
UNMAPPED_RUNTIME_TYPES = {11: 3, 115: 3, 120: 2, 122: 1, 123: 1}
CURRENT_POOL_CAPACITY = {'units': 128, 'buildings': 64, 'minerals': 32, 'geysers': 4}
EXPECTED_POOL_DEMAND = {'units': 74, 'buildings': 66, 'minerals': 39, 'geysers': 2}


def _load_module(path: Path, name: str):
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f'cannot load local helper: {path}')
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


def _sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def parse_chk_sections(chk: bytes) -> dict[bytes, bytes]:
    """Return exact CHK payloads, rejecting malformed or duplicate records."""
    sections: dict[bytes, bytes] = {}
    offset = 0
    while offset < len(chk):
        if len(chk) - offset < 8:
            raise ValueError('CHK has a truncated section header')
        tag = chk[offset:offset + 4]
        size = int.from_bytes(chk[offset + 4:offset + 8], 'little')
        end = offset + 8 + size
        if end > len(chk):
            raise ValueError(f'CHK section {tag!r} exceeds source bounds')
        if tag in sections:
            raise ValueError(f'CHK has duplicate {tag!r} sections')
        sections[tag] = chk[offset + 8:end]
        offset = end
    return sections


def source_type_counts(unit_rows: list[dict[str, int]]) -> dict[int, int]:
    """Validate row types and return the complete source UNIT histogram."""
    counts = Counter()
    for index, row in enumerate(unit_rows):
        raw_type = row.get('source_type')
        if not isinstance(raw_type, int) or not 0 <= raw_type <= 0xffff:
            raise ValueError(f'UNIT row {index} has an invalid source type')
        counts[raw_type] += 1
    if dict(counts) != EXPECTED_SOURCE_TYPES:
        raise ValueError('Z4 UNIT source rows differ from the audited complete histogram')
    return dict(sorted(counts.items()))


def source_pool_demand(type_counts: dict[int, int]) -> dict[str, int]:
    """Classify every retained placement into current bounded runtime pools."""
    demand = {
        'units': sum(count for raw, count in type_counts.items()
                     if raw not in BUILDING_TYPES | MINERAL_TYPES | GEYSER_TYPES | {START_TYPE}),
        'buildings': sum(count for raw, count in type_counts.items()
                         if raw in BUILDING_TYPES),
        'minerals': sum(count for raw, count in type_counts.items()
                        if raw in MINERAL_TYPES),
        'geysers': sum(count for raw, count in type_counts.items()
                       if raw in GEYSER_TYPES),
    }
    if demand != EXPECTED_POOL_DEMAND:
        raise ValueError('Z4 source pool demand differs from the audited complete source')
    return demand


def audit_source(info, chk: bytes, importer, extractor) -> dict:
    """Authenticate and summarize the complete Z4 source without truncation."""
    if info.bolt_path != BOLT_PATH:
        raise ValueError(f'expected BOLT {BOLT_PATH}, found {info.bolt_path}')
    if (info.width, info.height) != EXPECTED_DIMENSIONS:
        raise ValueError('Z4 dimensions differ from the audited 96x96 source')
    if (info.tileset & 7) != EXPECTED_TILESET_ID:
        raise ValueError('Z4 tileset differs from the audited Ashworld source')
    chk_hash = _sha256(chk)
    if chk_hash != CHK_SHA256:
        raise ValueError('Z4 CHK does not match the authenticated source hash')

    sections = parse_chk_sections(chk)
    missing = [tag.decode('ascii').strip() for tag in RETAINED_SECTIONS
               if tag not in sections]
    if missing:
        raise ValueError(f'Z4 CHK is missing required source sections: {missing}')
    for tag, expected_size in EXPECTED_RECORD_COUNTS.items():
        if len(sections.get(tag, b'')) != expected_size:
            raise ValueError(f'{tag.decode("ascii").strip()} size differs from audited Z4 source')

    rows = importer.unit_rows(sections[b'UNIT'])
    type_counts = source_type_counts(rows)
    pool_demand = source_pool_demand(type_counts)
    if len(rows) != 184 or sum(row['source_type'] == START_TYPE for row in rows) != 3:
        raise ValueError('Z4 UNIT row or start-marker count differs from the audited source')
    for index, row in enumerate(rows):
        if not (0 <= row['x_source_px'] < info.width * 32 and
                0 <= row['y_source_px'] < info.height * 32):
            raise ValueError(f'Z4 UNIT row {index} has coordinates outside the source map')
        if not 0 <= row['owner_raw'] < 12:
            raise ValueError(f'Z4 UNIT row {index} has an invalid source owner')

    owners = list(sections[b'OWNR'])
    sides = list(sections[b'SIDE'])
    forces = list(sections[b'FORC'])
    if owners[3] != 6 or sum(owner == 6 for owner in owners) != 1:
        raise ValueError('Z4 source setup no longer has one human slot at player 4')
    triggers = importer.trigger_rows(sections[b'TRIG'], sections[b'STR '], extractor)
    if len(triggers) != 7:
        raise ValueError('Z4 trigger count differs from the audited source')
    action_counts = Counter(action['name'] for trigger in triggers
                            for action in trigger['actions'])
    condition_counts = Counter(condition['name'] for trigger in triggers
                               for condition in trigger['conditions'])
    required_actions = {'CreateUnitwithProperties', 'RunAIScriptAtLocation',
                        'SetCountdownTimer', 'SetMissionObjectives', 'Victory', 'Defeat'}
    if not required_actions.issubset(action_counts):
        raise ValueError('Z4 trigger program is missing an audited action family')
    locations = []
    for index in range(len(sections[b'MRGN']) // 20):
        raw = sections[b'MRGN'][index * 20:(index + 1) * 20]
        left, top, right, bottom, name_id, flags = struct.unpack('<IIIIHH', raw)
        if left or top or right or bottom or name_id:
            locations.append({'index': index + 1,
                              'bounds_source_pixels': [left, top, right, bottom],
                              'name_string_id': name_id, 'flags': flags})
    objectives = [action.get('text', '') for trigger in triggers
                  for action in trigger['actions'] if action['name'] == 'SetMissionObjectives']
    return {
        'schema': 1,
        'mission': {'id': MISSION_ID, 'global_catalog_index': GLOBAL_CATALOG_INDEX,
                    'bolt_path': BOLT_PATH, 'title': info.name,
                    'dimensions': [info.width, info.height],
                    'tileset_id': info.tileset & 7, 'tileset': info.tileset_name},
        'source': {'normalized_rom_sha1': ROM_SHA1,
                   'extractor_commit': EXTRACTOR_COMMIT,
                   'chk_sha256': chk_hash},
        'source_setup': {'OWNR': owners, 'SIDE': sides, 'FORC': forces},
        'placement_audit': {'unit_rows_including_start_markers': len(rows),
                            'start_markers': 3, 'source_type_counts':
                            {str(key): value for key, value in type_counts.items()},
                            'source_pool_demand': pool_demand,
                            'current_pool_capacity_reference': CURRENT_POOL_CAPACITY,
                            'capacity_excess': {
                                key: max(0, pool_demand[key] - CURRENT_POOL_CAPACITY[key])
                                for key in pool_demand},
                            'unmapped_runtime_source_types': {
                                str(key): count for key, count in UNMAPPED_RUNTIME_TYPES.items()},
                            'all_rows_retained': True},
        'trigger_audit': {'trigger_count': len(triggers),
                          'condition_counts': dict(sorted(condition_counts.items())),
                          'action_counts': dict(sorted(action_counts.items())),
                          'objective_text': objectives,
                          'trigger_records': triggers},
        'locations': locations,
        'runtime_status': 'source-only staging; no authored Z4 runtime or playable claim',
        'limitations': [
            'No generic CHK trigger interpreter or generic AI script runner.',
            'Source roster exceeds the current 64-building and 32-mineral pools.',
            'Rows without a current gameplay mapping remain preserved as raw source records.',
        ],
        'section_hashes': {tag.decode('ascii').strip(): _sha256(payload)
                           for tag, payload in sorted(sections.items())},
    }


def _verify_extractor_checkout(importer) -> None:
    if importer.UPSTREAM_COMMIT != EXTRACTOR_COMMIT:
        raise ValueError('sc64_import.py does not name the expected pinned extractor commit')
    try:
        result = subprocess.run(
            ['git', '-C', str(importer.UPSTREAM.parent), 'rev-parse', 'HEAD'],
            check=True, capture_output=True, text=True)
    except (OSError, subprocess.CalledProcessError) as exc:
        raise ValueError('cannot verify the local pinned extractor checkout') from exc
    if result.stdout.strip() != EXTRACTOR_COMMIT:
        raise ValueError('local extractor checkout is not at the pinned revision')
    try:
        relative_path = importer.UPSTREAM.resolve().relative_to(
            importer.UPSTREAM.parent.resolve())
        extractor_source = subprocess.run(
            ['git', '-C', str(importer.UPSTREAM.parent), 'show',
             f'{EXTRACTOR_COMMIT}:{relative_path.as_posix()}'],
            check=True, capture_output=True)
        working_source = importer.UPSTREAM.read_bytes()
    except (OSError, ValueError, subprocess.CalledProcessError) as exc:
        raise ValueError('cannot verify pinned extractor source bytes') from exc
    if working_source != extractor_source.stdout:
        raise ValueError('local extractor source differs from the pinned revision')


def pack(rom_path: Path = DEFAULT_ROM, output_dir: Path = DEFAULT_OUT) -> dict:
    importer = _load_module(ROOT / 'tools/sc64_import.py', 'sc64_z4_source_import')
    _verify_extractor_checkout(importer)
    extractor = importer.upstream_module(importer.UPSTREAM)
    rom = extractor.load_rom(str(rom_path))
    rom_sha1 = hashlib.sha1(rom).hexdigest()
    if rom_sha1 != ROM_SHA1:
        raise ValueError('ROM does not match the verified StarCraft 64 USA image')
    archive = extractor.BoltArchive(rom)
    selected = [(info, chk) for info, chk in extractor.find_maps(archive)
                if info.bolt_path == BOLT_PATH]
    if len(selected) != 1:
        raise ValueError(f'expected one Z4 map at {BOLT_PATH}; found {len(selected)}')
    info, chk = selected[0]
    audit = audit_source(info, chk, importer, extractor)

    output_dir.mkdir(parents=True, exist_ok=True)
    records_dir = output_dir / 'records'
    records_dir.mkdir(parents=True, exist_ok=True)
    sections = parse_chk_sections(chk)
    record_manifest = {}
    for tag, filename in RETAINED_SECTIONS.items():
        payload = sections[tag]
        framed = tag + len(payload).to_bytes(4, 'little') + payload
        (records_dir / filename).write_bytes(framed)
        record_manifest[tag.decode('ascii').strip()] = {
            'file': filename, 'record_bytes': len(framed),
            'payload_bytes': len(payload), 'record_sha256': _sha256(framed),
            'payload_sha256': _sha256(payload),
        }
    (output_dir / 'source.chk').write_bytes(chk)
    manifest = {
        'schema': 1,
        'mission': {'id': MISSION_ID, 'global_catalog_index': GLOBAL_CATALOG_INDEX,
                    'source_map_path': BOLT_PATH},
        'source_rom_sha1': ROM_SHA1,
        'source_chk_sha256': CHK_SHA256,
        'extractor': 'references/sc64-maps/extract_sc64_maps.py',
        'extractor_commit': EXTRACTOR_COMMIT,
        'source_chk_file': 'source.chk',
        'source_chk_bytes': len(chk),
        'records': record_manifest,
    }
    (output_dir / 'manifest.json').write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + '\n', encoding='ascii')
    (output_dir / 'source-audit.json').write_text(
        json.dumps(audit, indent=2, ensure_ascii=True) + '\n', encoding='utf-8')
    staged = verify_staged(output_dir)
    return {'output': str(output_dir), 'source_chk_sha256': CHK_SHA256,
            'unit_rows': 184, 'triggers': 7, 'sections_retained': len(record_manifest),
            'verified_sections': staged['sections_verified']}


def verify_staged(output_dir: Path) -> dict:
    """Recheck the private retained CHK and every framed source record."""
    try:
        manifest = json.loads((output_dir / 'manifest.json').read_text(encoding='ascii'))
    except (OSError, json.JSONDecodeError) as exc:
        raise ValueError(f'cannot read staged Z4 source manifest: {exc}') from exc
    if (manifest.get('schema') != 1 or
            manifest.get('mission') != {'id': MISSION_ID,
                                        'global_catalog_index': GLOBAL_CATALOG_INDEX,
                                        'source_map_path': BOLT_PATH}):
        raise ValueError('staged manifest does not describe catalog Z4 at BOLT 008/017')
    if manifest.get('source_chk_file') != 'source.chk':
        raise ValueError('staged manifest names an unexpected source CHK file')
    try:
        source_chk = (output_dir / 'source.chk').read_bytes()
    except OSError as exc:
        raise ValueError(f'cannot read staged Z4 source CHK: {exc}') from exc
    if manifest.get('source_rom_sha1') != ROM_SHA1 or manifest.get('extractor_commit') != EXTRACTOR_COMMIT:
        raise ValueError('staged manifest has an unauthenticated ROM or extractor')
    if manifest.get('source_chk_sha256') != CHK_SHA256 or _sha256(source_chk) != CHK_SHA256:
        raise ValueError('staged Z4 CHK hash does not match the authenticated source')
    if manifest.get('source_chk_bytes') != len(source_chk):
        raise ValueError('staged source CHK size disagrees with its manifest')
    sections = parse_chk_sections(source_chk)
    records = manifest.get('records')
    if not isinstance(records, dict) or len(records) != len(RETAINED_SECTIONS):
        raise ValueError('staged manifest does not contain the complete retained record set')
    for tag, filename in RETAINED_SECTIONS.items():
        name = tag.decode('ascii').strip()
        metadata = records.get(name)
        if not isinstance(metadata, dict) or metadata.get('file') != filename:
            raise ValueError(f'staged manifest is missing {name}')
        try:
            framed = (output_dir / 'records' / filename).read_bytes()
        except OSError as exc:
            raise ValueError(f'cannot read staged {name}: {exc}') from exc
        expected = tag + len(sections[tag]).to_bytes(4, 'little') + sections[tag]
        if framed != expected:
            raise ValueError(f'staged {name} differs from the authenticated source CHK')
        if (metadata.get('record_bytes') != len(framed) or
                metadata.get('payload_bytes') != len(sections[tag]) or
                metadata.get('record_sha256') != _sha256(framed) or
                metadata.get('payload_sha256') != _sha256(sections[tag])):
            raise ValueError(f'staged {name} record integrity check failed')
    return {'source_chk_sha256': CHK_SHA256, 'sections_verified': len(records)}


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rom', type=Path,
                        help='verified owned source ROM (default: local prepared ROM)')
    parser.add_argument('--out', type=Path, default=DEFAULT_OUT)
    parser.add_argument('--verify-staged', action='store_true',
                        help='verify the staged source under --out without loading the ROM')
    args = parser.parse_args(argv)
    if args.verify_staged and args.rom is not None:
        parser.error('--verify-staged cannot be combined with --rom')
    if args.verify_staged:
        try:
            result = verify_staged(args.out)
        except (OSError, ValueError, RuntimeError, KeyError) as exc:
            print(f'sc64_z4_pack: {exc}', file=sys.stderr)
            return 1
        print(json.dumps({'output': str(args.out), 'verified_sections':
                          result['sections_verified'], 'source_chk_sha256':
                          result['source_chk_sha256']}, sort_keys=True))
        return 0
    rom_path = args.rom or DEFAULT_ROM
    if not rom_path.is_file():
        parser.error(f'ROM does not exist: {rom_path}')
    try:
        result = pack(rom_path, args.out)
    except (OSError, ValueError, RuntimeError, KeyError) as exc:
        print(f'sc64_z4_pack: {exc}', file=sys.stderr)
        return 1
    print(json.dumps(result, sort_keys=True))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
