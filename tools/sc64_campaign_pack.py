#!/usr/bin/env python3
"""Pack private Wasteland and Backwater terrain/unit data for one campaign ROM.

Backwater supplies the runtime palette and its existing T2 tables. This tool
generates only the additional Wasteland tables and a two-entry map descriptor.
All outputs stay under ignored .local/; source game data is never tracked.
"""
from __future__ import annotations

import argparse
from collections import Counter
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_ROM = ROOT / '.local/sc64/rom/StarCraft64-USA.z64'
DEFAULT_BASE = ROOT / '.local/sc64/backwater/generated'
DEFAULT_OUTPUT = ROOT / '.local/sc64/campaign/generated/sc64_campaign_generated.h'
ROM_SHA1 = 'a36d91b5aac7bb95df0d7de5f9814f5270e25c2b'
UPSTREAM_COMMIT = '09b57f787a247107d1de82b8c5eab2734667ea1d'


def load_module(name: str, path: Path):
    spec = importlib.util.spec_from_file_location(name, path)
    if not spec or not spec.loader:
        raise RuntimeError(f'Cannot load helper module: {path}')
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


def nearest_palette_index(color: int, target_palette: list[int], candidates: list[int]) -> int:
    def distance(candidate: int) -> int:
        return sum((((color >> shift) & 31) - ((candidate >> shift) & 31)) ** 2
                   for shift in (0, 5, 10))
    return min(candidates, key=lambda index: (distance(target_palette[index]), index))


def c_array(name: str, ctype: str, values, width: int = 24) -> str:
    values = list(values)
    rows = [','.join(str(v) for v in values[i:i + width])
            for i in range(0, len(values), width)]
    return (f'static const {ctype} {name}[{len(values)}] = {{\n'
            + ',\n'.join(rows) + '\n};\n')


def c_units(units: list[dict[str, int]]) -> str:
    result = [f'static const Sc64ImportedUnit sc64_campaign_t1_units[{len(units)}] = {{\n']
    for unit in units:
        if unit['flags'] > 0xFFFF or unit['resources'] > 0xFFFF:
            raise ValueError('T1 UNIT flags/resources exceed the shared imported-unit schema')
        result.append(
            f"    {{{unit['x_source_px'] // 2}u, {unit['y_source_px'] // 2}u, "
            f"{unit['source_type']}u, {unit['owner_raw']}u, {unit['hp_percent']}u, "
            f"{unit['flags']}u, {unit['resources']}u}},\n"
        )
    result.append('};\n')
    return ''.join(result)


def convert_wasteland(rom_path: Path, base: Path) -> tuple[dict, list[dict[str, int]], list[int], list[int], list[int], list[bytes]]:
    importer = load_module('sc64_campaign_import', ROOT / 'tools/sc64_import.py')
    assets = load_module('sc64_campaign_assets', ROOT / 'tools/sc64_assets.py')
    extractor = importer.upstream_module(importer.UPSTREAM)
    rom = extractor.load_rom(str(rom_path))
    digest = hashlib.sha1(rom).hexdigest()
    if digest != ROM_SHA1:
        raise ValueError(f'Expected verified USA ROM ({ROM_SHA1}), got {digest}')
    archive = extractor.BoltArchive(rom)
    selected = next(((info, chk) for info, chk in extractor.find_maps(archive)
                     if info.bolt_path == '008/00A'), None)
    if selected is None:
        raise ValueError('Wasteland mission 008/00A was not found in the verified ROM')
    info, chk = selected
    sections = importer.sections(chk, extractor)
    raw_terrain = sections.get(b'MTXM', b'')
    if len(raw_terrain) != info.width * info.height * 2:
        raise ValueError('Wasteland MTXM length does not match its map dimensions')
    if (info.width, info.height, info.tileset & 7) != (64, 64, 0):
        raise ValueError('Wasteland pack currently requires a 64x64 Badlands map')
    tile_ids = list(struct.unpack(f'<{len(raw_terrain) // 2}H', raw_terrain))

    entries = {entry.path: entry for entry in archive.entries()}
    def read(path: str) -> bytes:
        try:
            return archive.read(entries[path])
        except KeyError as exc:
            raise ValueError(f'Missing BOLT terrain dependency {path}') from exc

    source = {label: read(f'00B/{index:03X}') for label, index in
              [('vr4', 0), ('vx4', 1), ('vf4', 2), ('cv5', 3), ('palette', 4)]}
    decoded = {tile: assets.decode_tile(tile, source['cv5'], source['vx4'],
                                        source['vf4'], source['vr4'])
               for tile in set(tile_ids)}
    weighted = Counter()
    for tile, count in Counter(tile_ids).items():
        for pixel, occurrences in Counter(decoded[tile][0]).items():
            weighted[pixel] += occurrences * count
    t1_palette, source_remap = assets.compact_palette(
        assets.palette_rgb555(source['palette']), weighted)

    palette_file = base / 'palette.json'
    remap_file = base / 'palette-remap.bin'
    if not palette_file.is_file() or not remap_file.is_file():
        raise ValueError('Backwater generated palette/remap is missing; prepare Backwater assets first')
    t2_palette = json.loads(palette_file.read_text(encoding='utf-8'))['gba_rgb555']
    t2_source_remap = remap_file.read_bytes()
    if len(t2_palette) != 256 or len(t2_source_remap) != 256:
        raise ValueError('Backwater palette/remap does not have 256 entries')
    # Only indices actually used by the T2 map are eligible, so padded palette
    # slots cannot accidentally win as black when the palette has fewer colours.
    target_indices = sorted({index for index in t2_source_remap if 32 <= index <= 255})
    if not target_indices:
        raise ValueError('Backwater terrain remap contains no world palette colors')
    color_map = bytearray(256)
    for source_index, color in enumerate(t1_palette):
        color_map[source_index] = nearest_palette_index(color, t2_palette, target_indices)

    tiles: list[bytes] = []
    lookup: list[int] = []
    blocked: list[int] = []
    deduplicated: dict[bytes, int] = {}
    for tile_id in tile_ids:
        pixels, walk_mask = decoded[tile_id]
        packed = bytes(source_remap[pixel] for pixel in pixels).translate(color_map)
        index = deduplicated.get(packed)
        if index is None:
            index = len(tiles)
            deduplicated[packed] = index
            tiles.append(packed)
        lookup.append(index)
        blocked.append(int((walk_mask & 0x0660) != 0x0660))

    units = importer.unit_rows(sections.get(b'UNIT', b''))
    runtime_units = [unit for unit in units if unit['source_type'] != 214]
    mission = {
        'path': info.bolt_path,
        'title': info.name,
        'width': info.width,
        'height': info.height,
        'tileset': info.tileset_name,
        'chk_sha1': hashlib.sha1(chk).hexdigest(),
        'raw_unit_records': len(units),
        'runtime_unit_count': len(runtime_units),
        'source_tile_count': len(tile_ids),
        'unique_source_tiles': len(decoded),
        'unique_gba_tiles_after_palette_merge': len(tiles),
        'blocked_cells': sum(blocked),
        'source_assets_sha256': {name: hashlib.sha256(data).hexdigest()
                                 for name, data in source.items()},
    }
    return mission, runtime_units, tile_ids, lookup, blocked, tiles


def pack(rom_path: Path = DEFAULT_ROM, base: Path = DEFAULT_BASE,
         output: Path = DEFAULT_OUTPUT) -> dict:
    mission, units, _, lookup, blocked, tiles = convert_wasteland(rom_path, base)
    lines = [
        '/* Generated privately by tools/sc64_campaign_pack.py; do not commit. */\n',
        '#ifndef SC64_CAMPAIGN_GENERATED_H\n#define SC64_CAMPAIGN_GENERATED_H\n',
        '#include "sc64_types.h"\n',
        '#include "sc64_mission_generated.h"\n',
        '#include "sc64_assets_generated.h"\n',
        '#define SC64_CAMPAIGN_COUNT 2\n',
        f'#define SC64_CAMPAIGN_T1_UNIT_COUNT {len(units)}\n',
        f'#define SC64_CAMPAIGN_T1_TILE_COUNT {len(tiles)}\n',
        '#define SC64_CAMPAIGN_MAP_CELLS (64 * 64)\n',
        '/* T1 terrain pixels are remapped into Backwater shared palette. */\n',
        c_units(units),
        c_array('sc64_campaign_t1_tile_lookup', 'u16', lookup),
        c_array('sc64_campaign_t1_terrain_blocked', 'u8', blocked),
        f'static const u8 sc64_campaign_t1_tile_pixels[SC64_CAMPAIGN_T1_TILE_COUNT][256] __attribute__((aligned(4))) = {{\n',
    ]
    for tile in tiles:
        lines.append('{' + ','.join(map(str, tile)) + '},\n')
    lines.extend([
        '};\n',
        'typedef struct {\n',
        '    const Sc64ImportedUnit *units;\n',
        '    int unit_count;\n',
        '    const u8 (*pixels)[256];\n',
        '    const u16 *lookup;\n',
        '    const u8 *blocked;\n',
        '    const char *title;\n',
        '} Sc64CampaignMap;\n',
        'static const Sc64CampaignMap sc64_campaign_maps[SC64_CAMPAIGN_COUNT] = {\n',
        '    {sc64_campaign_t1_units, SC64_CAMPAIGN_T1_UNIT_COUNT, sc64_campaign_t1_tile_pixels, sc64_campaign_t1_tile_lookup, sc64_campaign_t1_terrain_blocked, "T1) Wasteland"},\n',
        '    {sc64_units, SC64_IMPORTED_UNIT_COUNT, sc64_tile_pixels, sc64_tile_lookup, sc64_terrain_blocked, "T2) Backwater Station"},\n',
        '};\n',
        '#endif /* SC64_CAMPAIGN_GENERATED_H */\n',
    ])
    content = ''.join(lines)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(content, encoding='ascii')
    report = {
        'schema': 1,
        'source': {
            'rom_filename': rom_path.name,
            'normalized_rom_sha1': ROM_SHA1,
            'extractor_commit': UPSTREAM_COMMIT,
            'palette_profile': 'Backwater Station (008/00B)',
        },
        'campaign_maps': [
            {'index': 0, **mission},
            {'index': 1, 'path': '008/00B', 'title': 'T2) Backwater Station',
             'width': 64, 'height': 64, 'source': 'references/sc64-maps/extract_sc64_maps.py'},
        ],
        'header': str(output),
        'header_sha256': hashlib.sha256(content.encode('ascii')).hexdigest(),
        'palette_merge': {
            'runtime_palette_indices': len(set(target_indices_from_remap(base))),
            'policy': 'Each Wasteland palette color maps to its nearest used Backwater RGB555 terrain color (indices 32..255); VF4 collision mask is retained.',
        },
    }
    report_path = output.with_name('sc64_campaign_pack_report.json')
    report_path.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    return report


def target_indices_from_remap(base: Path) -> list[int]:
    values = (base / 'palette-remap.bin').read_bytes()
    if len(values) != 256:
        raise ValueError('Backwater palette remap must contain 256 bytes')
    return sorted({index for index in values if 32 <= index <= 255})


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rom', type=Path, default=DEFAULT_ROM)
    parser.add_argument('--backwater-generated', type=Path, default=DEFAULT_BASE)
    parser.add_argument('--out', type=Path, default=DEFAULT_OUTPUT)
    args = parser.parse_args()
    if not args.rom.is_file():
        parser.error(f'ROM does not exist: {args.rom}')
    try:
        report = pack(args.rom, args.backwater_generated, args.out)
    except (OSError, ValueError, RuntimeError, KeyError) as error:
        print(f'sc64_campaign_pack: {error}', file=sys.stderr)
        raise SystemExit(1)
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
