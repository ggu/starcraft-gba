#!/usr/bin/env python3
"""Authenticate and privately stage the P4 source scenario from an owned ROM.

All extracted game data stays under ignored .local/. This is an audit/staging
tool only; it does not create runtime data or claim that P4 is playable.
"""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_ROM = ROOT / '.local/sc64/rom/StarCraft64-USA.z64'
DEFAULT_OUT = ROOT / '.local/sc64/p4'
ROM_SHA1 = 'a36d91b5aac7bb95df0d7de5f9814f5270e25c2b'
EXTRACTOR_COMMIT = '09b57f787a247107d1de82b8c5eab2734667ea1d'
BOLT_PATH = '008/021'
MISSION_ID = 'P4'
GLOBAL_CATALOG_INDEX = 23
CHK_SHA256 = '40f30ccedf87a3b9c88308dc3092611ceff10185ddaf32471104902042a93ecd'
EXPECTED_DIMENSIONS = (128, 128)
EXPECTED_TILESET_ID = 1
EXPECTED_UNIT_ROWS = 201
EXPECTED_TRIGGER_ROWS = 13
UNIT_RECORD_BYTES = 36
TRIGGER_RECORD_BYTES = 2400

# Full source UNIT histogram; the CHK remains authoritative and is retained in
# full. Keeping this count exact prevents accidental omission/reclassification.
EXPECTED_SOURCE_TYPES = {
    37: 77, 38: 21, 41: 2, 42: 13, 43: 5, 45: 2, 64: 4, 65: 6,
    66: 4, 67: 2, 79: 1, 132: 1, 135: 1, 137: 1, 138: 1, 139: 1,
    140: 1, 142: 1, 144: 5, 146: 9, 149: 1, 154: 1, 156: 6, 157: 1,
    160: 1, 166: 1, 176: 6, 177: 12, 178: 8, 196: 1, 214: 5,
}

# Copies of complete framed records useful for independent inspection. source.chk
# additionally preserves every other original section byte-for-byte.
RETAINED_SECTIONS = {
    b'DIM ': 'DIM.bin', b'ERA ': 'ERA.bin', b'MTXM': 'MTXM.bin',
    b'TILE': 'TILE.bin', b'ISOM': 'ISOM.bin', b'UNIT': 'UNIT.bin',
    b'OWNR': 'OWNR.bin', b'IOWN': 'IOWN.bin', b'SIDE': 'SIDE.bin',
    b'FORC': 'FORC.bin', b'MRGN': 'MRGN.bin', b'TRIG': 'TRIG.bin',
    b'STR ': 'STR.bin', b'SWNM': 'SWNM.bin', b'THG2': 'THG2.bin',
    b'DD2 ': 'DD2.bin', b'PUNI': 'PUNI.bin', b'UPRP': 'UPRP.bin',
    b'UPUS': 'UPUS.bin',
}
EXPECTED_SECTION_SIZES = {
    b'DIM ': 4, b'ERA ': 2, b'MTXM': 128 * 128 * 2,
    b'TILE': 128 * 128 * 2, b'ISOM': 67080, b'UNIT': 201 * 36,
    b'OWNR': 12, b'IOWN': 12, b'SIDE': 12, b'FORC': 20,
    b'MRGN': 64 * 20, b'TRIG': 13 * 2400, b'SWNM': 1024,
    b'THG2': 680, b'DD2 ': 832, b'PUNI': 5700, b'UPRP': 1280,
    b'UPUS': 64,
}
BUILDING_TYPES = range(106, 176)
MINERAL_TYPES = {176, 177, 178}
GEYSER_TYPES = {188}
START_TYPE = 214


def _sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def _load_module(path: Path, name: str):
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f'cannot load local helper: {path}')
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


def parse_chk_sections(chk: bytes) -> dict[bytes, bytes]:
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


def source_type_counts(rows: list[dict[str, int]]) -> dict[int, int]:
    counts: dict[int, int] = {}
    for index, row in enumerate(rows):
        raw = row.get('source_type')
        if not isinstance(raw, int) or not 0 <= raw <= 0xffff:
            raise ValueError(f'UNIT row {index} has an invalid source type')
        counts[raw] = counts.get(raw, 0) + 1
    if counts != EXPECTED_SOURCE_TYPES:
        raise ValueError('P4 UNIT rows differ from the authenticated complete histogram')
    return dict(sorted(counts.items()))


def pool_demand(counts: dict[int, int]) -> dict[str, int]:
    buildings = sum(n for raw, n in counts.items() if raw in BUILDING_TYPES)
    minerals = sum(n for raw, n in counts.items() if raw in MINERAL_TYPES)
    geysers = sum(n for raw, n in counts.items() if raw in GEYSER_TYPES)
    units = sum(n for raw, n in counts.items()
                if raw not in BUILDING_TYPES and raw not in MINERAL_TYPES
                and raw not in GEYSER_TYPES and raw != START_TYPE)
    return {'units': units, 'buildings': buildings,
            'minerals': minerals, 'geysers': geysers}


def audit_source(info, chk: bytes, importer, extractor) -> dict:
    if info.bolt_path != BOLT_PATH:
        raise ValueError(f'expected BOLT {BOLT_PATH}, found {info.bolt_path}')
    if (info.width, info.height) != EXPECTED_DIMENSIONS:
        raise ValueError('P4 dimensions differ from the authenticated 128x128 source')
    if (info.tileset & 7) != EXPECTED_TILESET_ID:
        raise ValueError('P4 tileset differs from the authenticated Space Platform source')
    if _sha256(chk) != CHK_SHA256:
        raise ValueError('P4 CHK does not match the authenticated source hash')
    sections = importer.sections(chk, extractor)
    missing = [tag.decode('ascii').strip() for tag in RETAINED_SECTIONS
               if tag not in sections]
    if missing:
        raise ValueError(f'P4 CHK is missing required source sections: {missing}')
    for tag, size in EXPECTED_SECTION_SIZES.items():
        if len(sections.get(tag, b'')) != size:
            raise ValueError(f'{tag.decode("ascii").strip()} size differs from authenticated P4')
    rows = importer.unit_rows(sections[b'UNIT'])
    counts = source_type_counts(rows)
    if len(rows) != EXPECTED_UNIT_ROWS or counts.get(START_TYPE, 0) != 5:
        raise ValueError('P4 UNIT row or start-marker count differs from authenticated source')
    for index, row in enumerate(rows):
        if not (0 <= row['x_source_px'] < info.width * 32 and
                0 <= row['y_source_px'] < info.height * 32):
            raise ValueError(f'P4 UNIT row {index} has coordinates outside the source map')
        if not 0 <= row['owner_raw'] < 12:
            raise ValueError(f'P4 UNIT row {index} has an invalid source owner')
    if len(sections[b'TRIG']) // TRIGGER_RECORD_BYTES != EXPECTED_TRIGGER_ROWS:
        raise ValueError('P4 trigger count differs from authenticated source')
    triggers = importer.trigger_rows(sections[b'TRIG'], sections[b'STR '], extractor)
    if len(triggers) != EXPECTED_TRIGGER_ROWS:
        raise ValueError('P4 nonempty trigger count differs from authenticated source')
    return {
        'schema': 1,
        'mission': {'id': MISSION_ID, 'global_catalog_index': GLOBAL_CATALOG_INDEX,
                    'bolt_path': BOLT_PATH, 'title': info.name,
                    'dimensions': [info.width, info.height],
                    'tileset_id': info.tileset & 7, 'tileset': info.tileset_name},
        'source': {'normalized_rom_sha1': ROM_SHA1,
                   'extractor_commit': EXTRACTOR_COMMIT, 'chk_sha256': CHK_SHA256},
        'source_setup': {'OWNR': list(sections[b'OWNR']),
                         'IOWN': list(sections[b'IOWN']),
                         'SIDE': list(sections[b'SIDE']),
                         'FORC': list(sections[b'FORC'])},
        'placement_audit': {
            'unit_rows_including_start_markers': len(rows),
            'start_markers': counts[START_TYPE],
            'source_type_counts': {str(k): v for k, v in counts.items()},
            'source_pool_demand': pool_demand(counts),
            'all_rows_retained': True,
            'building_classification': 'all raw source types 106..175 inclusive',
        },
        'trigger_audit': {'trigger_count': len(triggers),
                          'raw_trigger_bytes': len(sections[b'TRIG']),
                          'trigger_records_retained': True},
        'retained_sections': {tag.decode('ascii').strip(): {
            'bytes': len(payload), 'sha256': _sha256(payload)}
            for tag, payload in sorted(sections.items())},
        'runtime_status': 'source-only staging; no authored P4 runtime or playable claim',
    }


def _verify_extractor_checkout(importer) -> None:
    if importer.UPSTREAM_COMMIT != EXTRACTOR_COMMIT:
        raise ValueError('sc64_import.py does not name the expected pinned extractor commit')
    try:
        result = subprocess.run(['git', '-C', str(importer.UPSTREAM.parent),
                                 'rev-parse', 'HEAD'], check=True,
                                capture_output=True, text=True)
        committed = subprocess.run(
            ['git', '-C', str(importer.UPSTREAM.parent), 'show',
             f'{EXTRACTOR_COMMIT}:extract_sc64_maps.py'],
            check=True, capture_output=True)
        working = importer.UPSTREAM.read_bytes()
    except (OSError, subprocess.CalledProcessError) as exc:
        raise ValueError('cannot verify the local pinned extractor checkout') from exc
    if result.stdout.strip() != EXTRACTOR_COMMIT:
        raise ValueError('local extractor checkout is not at the pinned revision')
    if working != committed.stdout:
        raise ValueError('local extractor source differs from the pinned revision')


def pack(rom_path: Path = DEFAULT_ROM, output_dir: Path = DEFAULT_OUT) -> dict:
    importer = _load_module(ROOT / 'tools/sc64_import.py', 'sc64_p4_source_import')
    _verify_extractor_checkout(importer)
    extractor = importer.upstream_module(importer.UPSTREAM)
    rom = extractor.load_rom(str(rom_path))
    if hashlib.sha1(rom).hexdigest() != ROM_SHA1:
        raise ValueError('ROM does not match the verified StarCraft 64 USA image')
    maps = [(info, chk) for info, chk in extractor.find_maps(extractor.BoltArchive(rom))
            if info.bolt_path == BOLT_PATH]
    if len(maps) != 1:
        raise ValueError(f'expected one P4 map at {BOLT_PATH}; found {len(maps)}')
    info, chk = maps[0]
    audit = audit_source(info, chk, importer, extractor)
    output_dir.mkdir(parents=True, exist_ok=True)
    records_dir = output_dir / 'records'
    records_dir.mkdir(parents=True, exist_ok=True)
    sections = parse_chk_sections(chk)
    records = {}
    for tag, filename in RETAINED_SECTIONS.items():
        payload = sections[tag]
        framed = tag + len(payload).to_bytes(4, 'little') + payload
        (records_dir / filename).write_bytes(framed)
        records[tag.decode('ascii').strip()] = {
            'file': filename, 'record_bytes': len(framed),
            'payload_bytes': len(payload), 'record_sha256': _sha256(framed),
            'payload_sha256': _sha256(payload)}
    (output_dir / 'source.chk').write_bytes(chk)
    manifest = {'schema': 1,
                'mission': {'id': MISSION_ID,
                            'global_catalog_index': GLOBAL_CATALOG_INDEX,
                            'source_map_path': BOLT_PATH},
                'source_rom_sha1': ROM_SHA1,
                'source_chk_sha256': CHK_SHA256,
                'extractor': 'references/sc64-maps/extract_sc64_maps.py',
                'extractor_commit': EXTRACTOR_COMMIT,
                'source_chk_file': 'source.chk', 'source_chk_bytes': len(chk),
                'records': records}
    (output_dir / 'manifest.json').write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + '\n', encoding='ascii')
    (output_dir / 'source-audit.json').write_text(
        json.dumps(audit, indent=2, ensure_ascii=True) + '\n', encoding='utf-8')
    verified = verify_staged(output_dir)
    return {'output': str(output_dir), 'source_chk_sha256': CHK_SHA256,
            'unit_rows': EXPECTED_UNIT_ROWS, 'triggers': EXPECTED_TRIGGER_ROWS,
            'sections_retained': len(records), 'verified_sections': verified['sections_verified'],
            'source_pool_demand': audit['placement_audit']['source_pool_demand']}


def verify_staged(output_dir: Path) -> dict:
    try:
        manifest = json.loads((output_dir / 'manifest.json').read_text(encoding='ascii'))
        chk = (output_dir / 'source.chk').read_bytes()
    except (OSError, json.JSONDecodeError) as exc:
        raise ValueError(f'cannot read staged P4 source: {exc}') from exc
    expected_mission = {'id': MISSION_ID, 'global_catalog_index': GLOBAL_CATALOG_INDEX,
                        'source_map_path': BOLT_PATH}
    if manifest.get('schema') != 1 or manifest.get('mission') != expected_mission:
        raise ValueError('staged manifest does not describe catalog P4 at BOLT 008/021')
    if manifest.get('source_chk_file') != 'source.chk':
        raise ValueError('staged manifest names an unexpected source CHK file')
    if (manifest.get('source_rom_sha1') != ROM_SHA1 or
            manifest.get('extractor_commit') != EXTRACTOR_COMMIT):
        raise ValueError('staged manifest has an unauthenticated ROM or extractor')
    if manifest.get('source_chk_sha256') != CHK_SHA256 or _sha256(chk) != CHK_SHA256:
        raise ValueError('staged P4 CHK hash does not match the authenticated source')
    if manifest.get('source_chk_bytes') != len(chk):
        raise ValueError('staged source CHK size disagrees with its manifest')
    sections = parse_chk_sections(chk)
    records = manifest.get('records')
    if not isinstance(records, dict) or set(records) != {
            tag.decode('ascii').strip() for tag in RETAINED_SECTIONS}:
        raise ValueError('staged manifest does not contain the complete retained record set')
    for tag, filename in RETAINED_SECTIONS.items():
        name = tag.decode('ascii').strip()
        meta = records[name]
        if not isinstance(meta, dict) or meta.get('file') != filename:
            raise ValueError(f'staged manifest has an invalid {name} filename')
        try:
            framed = (output_dir / 'records' / filename).read_bytes()
        except OSError as exc:
            raise ValueError(f'cannot read staged {name}: {exc}') from exc
        payload = sections[tag]
        expected = tag + len(payload).to_bytes(4, 'little') + payload
        if framed != expected:
            raise ValueError(f'staged {name} differs from authenticated source CHK')
        if (meta.get('record_bytes') != len(framed) or
                meta.get('payload_bytes') != len(payload) or
                meta.get('record_sha256') != _sha256(framed) or
                meta.get('payload_sha256') != _sha256(payload)):
            raise ValueError(f'staged {name} record integrity check failed')
    return {'source_chk_sha256': CHK_SHA256, 'sections_verified': len(records)}


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rom', type=Path, help='verified owned source ROM')
    parser.add_argument('--out', type=Path, default=DEFAULT_OUT)
    parser.add_argument('--verify-staged', action='store_true')
    args = parser.parse_args(argv)
    if args.verify_staged and args.rom is not None:
        parser.error('--verify-staged cannot be combined with --rom')
    try:
        result = (verify_staged(args.out) if args.verify_staged
                  else pack(args.rom or DEFAULT_ROM, args.out))
    except (OSError, ValueError, RuntimeError, KeyError) as exc:
        print(f'sc64_p4_pack: {exc}', file=sys.stderr)
        return 1
    print(json.dumps({'output': str(args.out), **result}, sort_keys=True))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
