#!/usr/bin/env python3
"""Pack authenticated Z5 THG2 records as numeric-only private scenario data.

All source bytes remain in ignored .local storage. This tool emits every
record in source order and marks unit-backed rows using the same THG2 sprite
bit used by the established T4 adapter. It does not emit art or text.
"""
from __future__ import annotations

import argparse
import importlib.util
import json
from pathlib import Path
import tempfile

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_SOURCE_ROOT = ROOT / '.local/sc64/zp-stage/generated/source_maps'
DEFAULT_OUTPUT = ROOT / '.local/sc64/zp-stage/generated/scenarios/sc64_z5_doodads_generated.h'
THG2_SPRITE_FLAG = 0x1000
THG2_DISABLED_FLAG = 0x8000
RECORD_SIZE = 10
MAX_DOODADS = 255


def _load_scenario_packer():
    path = ROOT / 'tools/sc64_zp_scenario_pack.py'
    spec = importlib.util.spec_from_file_location('sc64_z5_doodad_source', path)
    if spec is None or spec.loader is None:
        raise RuntimeError('cannot load authenticated ZP scenario decoder')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def decode_thg2(payload: bytes, width: int, height: int) -> list[dict]:
    """Decode complete numeric THG2 rows without discarding sprite records."""
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
        unit_backed = not bool(flags & THG2_SPRITE_FLAG)
        rows.append({'index': index, 'type': raw_type, 'x': x, 'y': y,
                     'owner': owner, 'unused': unused, 'flags': flags,
                     'unit_backed': int(unit_backed),
                     'initial_enabled': int(unit_backed and not (flags & THG2_DISABLED_FLAG))})
    return rows


def parse_source(source_root: Path) -> dict:
    """Authenticate complete staged Z5 CHK, then classify every THG2 row."""
    decoder = _load_scenario_packer()
    scenario = decoder.parse_source(source_root, 'Z5')
    source_dir = source_root / 'z5'
    try:
        chk = (source_dir / 'source.chk').read_bytes()
    except OSError as exc:
        raise ValueError(f'cannot read authenticated Z5 source CHK: {exc}') from exc
    sections = decoder.parse_chk_sections(chk)
    payload = sections.get(b'THG2')
    if payload is None:
        raise ValueError('authenticated Z5 CHK has no THG2 section')
    rows = decode_thg2(payload, scenario['width'], scenario['height'])
    return {'mission': 'Z5', 'global_catalog_id': scenario['global_catalog_id'],
            'source_map_path': scenario['source_map_path'],
            'source_chk_sha256': scenario['source_chk_sha256'],
            'width': scenario['width'], 'height': scenario['height'],
            'all_count': len(rows),
            'unit_backed_count': sum(row['unit_backed'] for row in rows),
            'sprite_count': sum(not row['unit_backed'] for row in rows),
            'doodads': rows}


def emit_header(data: dict) -> str:
    rows = data['doodads']
    out = [
        '#ifndef SC64_Z5_DOODADS_GENERATED_H',
        '#define SC64_Z5_DOODADS_GENERATED_H',
        '/* Authenticated numeric Z5 THG2 rows. Do not add source assets/text. */',
        f'#define SC64_Z5_DOODAD_GLOBAL_CATALOG_ID {data["global_catalog_id"]}u',
        f'#define SC64_Z5_DOODAD_MAP_WIDTH {data["width"]}u',
        f'#define SC64_Z5_DOODAD_MAP_HEIGHT {data["height"]}u',
        f'#define SC64_Z5_DOODAD_COUNT {len(rows)}u',
        f'#define SC64_Z5_UNIT_BACKED_DOODAD_COUNT {data["unit_backed_count"]}u',
        f'#define SC64_Z5_SPRITE_DOODAD_COUNT {data["sprite_count"]}u',
        'typedef struct { unsigned short type, x, y; unsigned char owner, unused; '
        'unsigned short flags; unsigned char unit_backed, initial_enabled; } Sc64Z5SourceDoodad;',
        'static const Sc64Z5SourceDoodad sc64_z5_source_doodads[SC64_Z5_DOODAD_COUNT] = {',
    ]
    for r in rows:
        out.append('    {%du,%du,%du,%du,%du,%du,%du,%du},' %
                   (r['type'], r['x'], r['y'], r['owner'], r['unused'], r['flags'],
                    r['unit_backed'], r['initial_enabled']))
    out += ['};', '#endif']
    return '\n'.join(out) + '\n'


def verify_staged(source_root: Path) -> dict:
    data = parse_source(source_root)
    return {key: data[key] for key in ('mission', 'global_catalog_id',
            'source_map_path', 'source_chk_sha256', 'width', 'height',
            'all_count', 'unit_backed_count', 'sprite_count')}


def generate(source_root: Path, output: Path) -> dict:
    data = parse_source(source_root)
    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile('w', encoding='ascii', dir=output.parent,
                                     prefix=output.name + '.', delete=False) as f:
        temp = Path(f.name)
        f.write(emit_header(data))
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
