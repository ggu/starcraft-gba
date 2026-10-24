#!/usr/bin/env python3
"""Pack authenticated P6 THG2 rows as private numeric-only metadata.

The packer preserves the complete source row and never emits sprites, strings,
or audio. Unsupported props remain present and explicitly marked unsupported so
runtime integration can fail closed instead of silently dropping them.
"""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import tempfile

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_SOURCE_ROOT = ROOT / '.local/sc64/zp-stage/generated/source_maps'
DEFAULT_OUTPUT = ROOT / '.local/sc64/p6/doodads.json'
THG2_SPRITE_FLAG = 0x1000
THG2_DISABLED_FLAG = 0x8000
RECORD_SIZE = 10
MAX_DOODADS = 255
P6_GLOBAL_ID = 25
P6_SOURCE_MAP_PATH = '008/023'

# Authenticated P6 counts by raw THG2 type. This checks that staging retains
# every source row, including sprite-only decoration and unsupported props.
EXPECTED_RAW_COUNTS = {
    120: 3, 121: 1, 122: 1, 124: 2, 127: 1, 128: 2,
    203: 2, 205: 5, 206: 1, 207: 2, 208: 4, 209: 6,
    210: 2, 211: 4, 213: 1,
}
# These unit-backed rows are retained, but this profile has no complete GBA
# prop policy for them. 121 and 128 are SpriteType IDs in this map, not
# UnitType IDs: THG2 marks all their occurrences sprite-only.
UNSUPPORTED_RAW_TYPES = frozenset((210,))
DOOR_RAW_TYPES = frozenset((205, 206, 207, 208))


def _load_scenario_packer():
    path = ROOT / 'tools/sc64_zp_scenario_pack.py'
    spec = importlib.util.spec_from_file_location('sc64_p6_doodad_scenario', path)
    if spec is None or spec.loader is None:
        raise RuntimeError('cannot load authenticated ZP scenario decoder')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def decode_thg2(payload: bytes, width: int, height: int) -> list[dict]:
    """Decode each complete THG2 row, preserving all numeric source fields."""
    if not isinstance(payload, bytes) or len(payload) % RECORD_SIZE:
        raise ValueError('THG2 payload has a partial source record')
    if not (0 < width <= 128 and 0 < height <= 128):
        raise ValueError('map dimensions are outside imported-map bounds')
    count = len(payload) // RECORD_SIZE
    if count > MAX_DOODADS:
        raise ValueError(f'THG2 count {count} exceeds byte-sized source index capacity')

    rows = []
    for index in range(count):
        start = index * RECORD_SIZE
        raw_type = int.from_bytes(payload[start:start + 2], 'little')
        x = int.from_bytes(payload[start + 2:start + 4], 'little')
        y = int.from_bytes(payload[start + 4:start + 6], 'little')
        owner, unused = payload[start + 6:start + 8]
        flags = int.from_bytes(payload[start + 8:start + 10], 'little')
        if x >= width * 32 or y >= height * 32:
            raise ValueError(f'THG2 row {index} has coordinates outside map bounds')

        sprite_only = bool(flags & THG2_SPRITE_FLAG)
        if sprite_only:
            disposition = 'sprite_only'
        elif raw_type in UNSUPPORTED_RAW_TYPES:
            disposition = 'unsupported'
        else:
            disposition = 'unit_backed_prop'

        # THG2's sprite bit says whether the source row is sprite-only. It is
        # independent from native UnitType::isBuilding(). P6's four door IDs
        # are Invincible but not Building in the pinned BWAPI UnitType flags;
        # classifying their THG2 rows in a GBA prop pool is runtime policy.
        rows.append({
            'index': index,
            'type': raw_type,
            'x': x,
            'y': y,
            'owner': owner,
            'unused': unused,
            'flags': flags,
            'source_kind': 'sprite_only' if sprite_only else 'unit_backed',
            'native_building': 0 if raw_type in DOOR_RAW_TYPES else None,
            'runtime_disposition': disposition,
            'initial_enabled': int(not (flags & THG2_DISABLED_FLAG)),
        })
    return rows


def _validate_p6_rows(rows: list[dict]) -> dict:
    counts: dict[int, int] = {}
    for row in rows:
        raw = row['type']
        counts[raw] = counts.get(raw, 0) + 1
    if counts != EXPECTED_RAW_COUNTS:
        raise ValueError('authenticated P6 THG2 type counts differ from the pinned inventory')
    unsupported = sorted(raw for raw in counts if raw in UNSUPPORTED_RAW_TYPES)
    return {
        'all_count': len(rows),
        'unit_backed_count': sum(row['source_kind'] == 'unit_backed' for row in rows),
        'sprite_count': sum(row['source_kind'] == 'sprite_only' for row in rows),
        'unsupported_count': sum(row['runtime_disposition'] == 'unsupported' for row in rows),
        'unsupported_raw_types': unsupported,
        'raw_type_counts': [{'type': raw, 'count': counts[raw]} for raw in sorted(counts)],
    }


def parse_source(source_root: Path) -> dict:
    """Authenticate the staged P6 CHK and preserve its full THG2 inventory."""
    decoder = _load_scenario_packer()
    scenario = decoder.parse_source(source_root, 'P6')
    if (scenario.get('mission') != 'P6' or
            scenario.get('global_catalog_id') != P6_GLOBAL_ID or
            scenario.get('source_map_path') != P6_SOURCE_MAP_PATH):
        raise ValueError('authenticated source is not the pinned P6 map')
    source_dir = source_root / 'p6'
    try:
        chk = (source_dir / 'source.chk').read_bytes()
    except OSError as exc:
        raise ValueError(f'cannot read authenticated P6 source CHK: {exc}') from exc
    if hashlib.sha256(chk).hexdigest() != scenario.get('source_chk_sha256'):
        raise ValueError('P6 CHK changed after scenario authentication')
    sections = decoder.parse_chk_sections(chk)
    payload = sections.get(b'THG2')
    if payload is None:
        raise ValueError('authenticated P6 CHK has no THG2 section')

    rows = decode_thg2(payload, scenario['width'], scenario['height'])
    summary = _validate_p6_rows(rows)
    return {
        'mission': 'P6',
        'global_catalog_id': P6_GLOBAL_ID,
        'source_map_path': P6_SOURCE_MAP_PATH,
        'source_chk_sha256': scenario['source_chk_sha256'],
        'width': scenario['width'],
        'height': scenario['height'],
        **summary,
        'doodads': rows,
    }


def require_runtime_supported(data: dict) -> None:
    """Reject use by a runtime while any retained prop lacks a GBA policy."""
    unsupported = data.get('unsupported_raw_types')
    if unsupported:
        raise ValueError(f'P6 has unsupported THG2 prop types: {unsupported}')


def verify_staged(source_root: Path) -> dict:
    data = parse_source(source_root)
    return {key: data[key] for key in (
        'mission', 'global_catalog_id', 'source_map_path', 'source_chk_sha256',
        'width', 'height', 'all_count', 'unit_backed_count', 'sprite_count',
        'unsupported_count', 'unsupported_raw_types', 'raw_type_counts')}


def generate(source_root: Path, output: Path) -> dict:
    """Write private numeric JSON metadata, never a runtime header or assets."""
    data = parse_source(source_root)
    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile('w', encoding='ascii', dir=output.parent,
                                     prefix=output.name + '.', delete=False) as f:
        temp = Path(f.name)
        json.dump(data, f, sort_keys=True, separators=(',', ':'))
        f.write('\n')
    temp.replace(output)
    return verify_staged(source_root)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-root', type=Path, default=DEFAULT_SOURCE_ROOT)
    parser.add_argument('--output', type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument('--verify-staged', action='store_true')
    args = parser.parse_args()
    try:
        result = verify_staged(args.source_root) if args.verify_staged else generate(args.source_root, args.output)
    except (OSError, ValueError, RuntimeError) as exc:
        parser.error(str(exc))
    print(json.dumps(result, sort_keys=True))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
