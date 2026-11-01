#!/usr/bin/env python3
"""Pack private N64 multiplayer terrain and start/resource data for skirmishes.

All generated ROM-derived data remains under ignored .local/. The output is
built from the user's verified StarCraft 64 ROM and the pinned extractor.

Copyright (C) 2026 blizzard-gba contributors
SPDX-License-Identifier: GPL-3.0-or-later
"""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import sys
from collections import deque

ROOT = Path(__file__).resolve().parents[1]
ROM = ROOT / '.local/sc64/rom/StarCraft64-USA.z64'
BASE = ROOT / '.local/sc64/backwater/generated'
OUTPUT = ROOT / '.local/sc64/skirmish/generated/sc64_skirmish_generated.h'
ROM_SHA1 = 'a36d91b5aac7bb95df0d7de5f9814f5270e25c2b'
UPSTREAM_COMMIT = '09b57f787a247107d1de82b8c5eab2734667ea1d'
MAPS = [('blood_bath', '008/04F')]
MINERAL_TYPES = {176, 177, 178}
GEYSER_TYPE = 188
START_TYPE = 214
MAX_MINERALS = 32
MINERALS_PER_START = 8
MAX_GEYSERS = 4
SUPPORTED_DIMENSIONS = {64, 96}


def load(name: str, path: Path):
    spec = importlib.util.spec_from_file_location(name, path)
    if not spec or not spec.loader:
        raise RuntimeError(f'Cannot load {path}')
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


def c_array(name: str, ctype: str, values, width: int = 24) -> str:
    values = list(values)
    rows = [','.join(str(v) for v in values[i:i + width])
            for i in range(0, len(values), width)]
    return f'static const {ctype} {name}[{len(values)}] = {{\n' + ',\n'.join(rows) + '\n};\n'


def nearest(color: int, palette: list[int], candidates: list[int]) -> int:
    def dist(i: int):
        p = palette[i]
        return sum((((color >> s) & 31) - ((p >> s) & 31)) ** 2 for s in (0, 5, 10))
    return min(candidates, key=lambda i: (dist(i), i))


def connected_component(blocked: list[int], width: int, height: int, start: int) -> set[int]:
    if not (0 <= start < width * height) or (blocked[start] & 1):
        raise ValueError('start marker is outside walkable terrain')
    seen = {start}
    queue = deque([start])
    while queue:
        cell = queue.popleft()
        x, y = cell % width, cell // width
        for nx, ny in ((x - 1, y), (x + 1, y), (x, y - 1), (x, y + 1)):
            if 0 <= nx < width and 0 <= ny < height:
                nxt = nx + ny * width
                if nxt not in seen and not (blocked[nxt] & 1):
                    seen.add(nxt)
                    queue.append(nxt)
    return seen


def validate_dimensions(path: str, width: int, height: int) -> None:
    if width not in SUPPORTED_DIMENSIONS or height not in SUPPORTED_DIMENSIONS:
        raise ValueError(f'{path} dimensions {width}x{height} are outside the supported 64/96-cell map bounds')


def spawn_clear(blocked: list[int], width: int, height: int, x_px: int, y_px: int) -> bool:
    """Check the authored 3x3-cell town hall footprint around a start marker."""
    cx, cy = x_px // 16, y_px // 16
    return all(0 <= x < width and 0 <= y < height and not (blocked[x + y * width] & 1)
               for y in range(cy - 1, cy + 2) for x in range(cx - 1, cx + 2))


def select_starts(blocked: list[int], width: int, height: int,
                  raw_starts: list[tuple[int, int]], wanted: int = 4) -> list[tuple[int, int]]:
    """Choose four legal, connected starts, favoring broad map coverage."""
    if len(raw_starts) < wanted:
        raise ValueError(f'map has only {len(raw_starts)} start markers; need {wanted}')
    candidates = []
    for x, y in raw_starts:
        cell = (x // 16) + (y // 16) * width
        if spawn_clear(blocked, width, height, x, y):
            candidates.append((x, y, cell))
    if len(candidates) < wanted:
        raise ValueError(f'only {len(candidates)} starts have clear 3x3 spawn footprints')
    # Greedy farthest-point selection, seeded by the northwest-most valid start.
    chosen = [min(candidates, key=lambda p: (p[1], p[0]))]
    while len(chosen) < wanted:
        remaining = [p for p in candidates if p not in chosen]
        chosen.append(max(remaining, key=lambda p: (
            min((p[0] - q[0]) ** 2 + (p[1] - q[1]) ** 2 for q in chosen), -p[1], -p[0])))
    components = [connected_component(blocked, width, height, p[2]) for p in chosen]
    if any(p[2] not in components[0] for p in chosen[1:]):
        raise ValueError('selected start locations are not mutually connected by VF4 walk cells')
    return [(x, y) for x, y, _ in chosen]


def choose_minerals(resources: list[dict], starts: list[tuple[int, int]]) -> tuple[list[dict], dict]:
    """Retain nearest eight distinct patches per selected start, bounded at 32."""
    patches = sorted(({'x': u['x_source_px'] // 2, 'y': u['y_source_px'] // 2,
                       'amount': min(u['resources'], 0xffff)}
                      for u in resources), key=lambda p: (p['x'], p['y'], p['amount']))
    selected = []
    seen = set()
    per_start = [0 for _ in starts]
    # Allocate one nearest patch per start per round. This preserves balanced
    # local access when the source has fewer than 32 patches.
    for _round in range(MINERALS_PER_START):
        for start_index, (sx, sy) in enumerate(starts):
            ranked = sorted(patches, key=lambda p: ((p['x'] - sx) ** 2 + (p['y'] - sy) ** 2,
                                                     p['y'], p['x']))
            for patch in ranked:
                key = (patch['x'], patch['y'])
                if key in seen:
                    continue
                if len(selected) >= MAX_MINERALS:
                    break
                selected.append(patch)
                seen.add(key)
                per_start[start_index] += 1
                break
            if len(selected) >= MAX_MINERALS:
                break
        if len(selected) >= MAX_MINERALS:
            break
    return selected, {'source_count': len(patches), 'packed_count': len(selected),
                      'omitted_count': len(patches) - len(selected),
                      'new_patches_selected_per_start': per_start,
                      'policy': f'nearest distinct patches, up to {MINERALS_PER_START} per selected start; bounded at {MAX_MINERALS}'}


def _emit_map(lines: list[str], index: int, slug: str, info, blocked: list[int],
              lookup: list[int], tiles: list[bytes], starts: list[tuple[int, int]],
              minerals: list[dict], geysers: list[dict]):
    prefix = f'sc64_sk_{slug}'
    lines.append(c_array(f'{prefix}_lookup', 'unsigned short', lookup))
    lines.append(c_array(f'{prefix}_blocked', 'unsigned char', blocked))
    lines.append(f'static const unsigned char {prefix}_tiles[{len(tiles)}][256] __attribute__((aligned(4))) = {{\n')
    lines.extend('{' + ','.join(map(str, tile)) + '},\n' for tile in tiles)
    lines.append('};\n')
    lines.append(f'static const unsigned short {prefix}_start_x[4] = {{{",".join(str(x) for x, _ in starts)}}};\n')
    lines.append(f'static const unsigned short {prefix}_start_y[4] = {{{",".join(str(y) for _, y in starts)}}};\n')
    padded_minerals = minerals + [{'x': 0, 'y': 0, 'amount': 0}] * (MAX_MINERALS - len(minerals))
    lines.append(f'static const Sc64SkirmishMineral {prefix}_minerals[{MAX_MINERALS}] = {{\n')
    lines.extend(f"    {{{p['x']}u,{p['y']}u,{p['amount']}u}},\n" for p in padded_minerals)
    lines.append('};\n')
    padded_geysers = geysers + [{'x': 0, 'y': 0}] * (MAX_GEYSERS - len(geysers))
    lines.append(f'static const Sc64SkirmishGeyser {prefix}_geysers[{MAX_GEYSERS}] = {{\n')
    lines.extend(f"    {{{g['x']}u,{g['y']}u}},\n" for g in padded_geysers)
    lines.append('};\n')
    return f'    {{{info.width}u,{info.height}u,{prefix}_tiles,{prefix}_lookup,{prefix}_blocked,{{{",".join(str(x) for x, _ in starts)}}},{{{",".join(str(y) for _, y in starts)}}},{prefix}_minerals,{len(minerals)}u,{prefix}_geysers,{len(geysers)}u,"{info.name}"}},\n'


def pack(rom_path: Path = ROM, base: Path = BASE, output: Path = OUTPUT, include_cover: bool = False) -> dict:
    importer = load('sc64_skirmish_import', ROOT / 'tools/sc64_import.py')
    assets = load('sc64_skirmish_assets', ROOT / 'tools/sc64_assets.py')
    x = importer.upstream_module(importer.UPSTREAM)
    rom = x.load_rom(str(rom_path))
    if hashlib.sha1(rom).hexdigest() != ROM_SHA1:
        raise ValueError('ROM does not match the verified StarCraft 64 USA image')
    archive = x.BoltArchive(rom)
    entries = {entry.path: entry for entry in archive.entries()}
    def read(path: str) -> bytes:
        try:
            return archive.read(entries[path])
        except KeyError as exc:
            raise ValueError(f'Missing BOLT terrain dependency {path}') from exc
    palette = json.loads((base / 'palette.json').read_text())['gba_rgb555']
    remap_data = (base / 'palette-remap.bin').read_bytes()
    if len(palette) != 256 or len(remap_data) != 256:
        raise ValueError('Prepare Backwater generated palette/remap before packing skirmish maps')
    shared = sorted({i for i in remap_data if i >= 32})
    if not shared:
        raise ValueError('Backwater sprite palette profile has no world colors')
    maps = {info.bolt_path: (info, chk) for info, chk in x.find_maps(archive)}
    lines = [
        '/* Private generated data. Do not commit or distribute. */\n',
        '/* Source: references/sc64-maps/extract_sc64_maps.py @ ' + UPSTREAM_COMMIT + ' */\n',
        '#ifndef SC64_SKIRMISH_GENERATED_H\n#define SC64_SKIRMISH_GENERATED_H\n',
        f'#define SC64_SKIRMISH_IMPORTED_COUNT {len(MAPS)}\n',
        'typedef struct { unsigned short x,y,amount; } Sc64SkirmishMineral;\n',
        'typedef struct { unsigned short x,y; } Sc64SkirmishGeyser;\n',
        'typedef struct { unsigned short width,height; const unsigned char (*tile_pixels)[256]; const unsigned short *tile_lookup; const unsigned char *blocked; unsigned short start_x[4],start_y[4]; const Sc64SkirmishMineral *minerals; unsigned char minerals_count; const Sc64SkirmishGeyser *geysers; unsigned char geyser_count; const char *title; } Sc64SkirmishMap;\n',
    ]
    reports = []
    descriptors = []
    for index, (slug, path) in enumerate(MAPS):
        if path not in maps:
            raise ValueError(f'Skirmish map {path} is absent from the verified ROM')
        info, chk = maps[path]
        validate_dimensions(path, info.width, info.height)
        sections = importer.sections(chk, x)
        raw = sections.get(b'MTXM', b'')
        if len(raw) != info.width * info.height * 2:
            raise ValueError(f'{path} MTXM dimensions do not match')
        tile_ids = list(struct.unpack(f'<{len(raw)//2}H', raw))
        tileset = info.tileset & 7
        asset_base = tileset * 0x0f
        sources = {label: read(f'00B/{asset_base + offset:03X}') for label, offset in
                   [('vr4', 0), ('vx4', 1), ('vf4', 2), ('cv5', 3), ('palette', 4)]}
        decoded = {tid: assets.decode_tile(tid, sources['cv5'], sources['vx4'],
                                           sources['vf4'], sources['vr4']) for tid in set(tile_ids)}
        remap = bytes(nearest(color, palette, shared) for color in assets.palette_rgb555(sources['palette']))
        pixels, lookup, blocked, dedup = [], [], [], {}
        for tid in tile_ids:
            pix, walk = decoded[tid]
            packed = pix.translate(remap)
            if packed not in dedup:
                dedup[packed] = len(pixels)
                pixels.append(packed)
            lookup.append(dedup[packed])
            blocked.append(int((walk & 0x0660) != 0x0660) | (assets.cv5_cover(tid, sources['cv5']) << 1 if include_cover else 0))
        units = importer.unit_rows(sections.get(b'UNIT', b''))
        raw_starts = [(u['x_source_px'] // 2, u['y_source_px'] // 2)
                      for u in units if u['source_type'] == START_TYPE]
        starts = select_starts(blocked, info.width, info.height, raw_starts)
        neutral = [u for u in units if u['owner_raw'] == 11]
        minerals, mineral_report = choose_minerals([u for u in neutral if u['source_type'] in MINERAL_TYPES], starts)
        source_geysers = [{'x': u['x_source_px'] // 2, 'y': u['y_source_px'] // 2}
                          for u in neutral if u['source_type'] == GEYSER_TYPE]
        if not source_geysers:
            raise ValueError(f'{path} has no source neutral geysers; refusing authored gas substitutions')
        if len(source_geysers) > MAX_GEYSERS:
            raise ValueError(f'{path} has {len(source_geysers)} geysers; runtime supports {MAX_GEYSERS} without truncation')
        geysers = source_geysers
        descriptors.append(_emit_map(lines, index, slug, info, blocked, lookup, pixels, starts, minerals, geysers))
        reports.append({'index': index + 1, 'path': path, 'title': info.name,
                        'dimensions': [info.width, info.height], 'tileset_id': tileset,
                        'selected_starts': [{'x': x, 'y': y} for x, y in starts],
                        'source_start_count': len(raw_starts), 'source_mineral_count': mineral_report['source_count'],
                        'packed_mineral_count': len(minerals), 'mineral_selection': mineral_report,
                        'source_geyser_count': len(source_geysers), 'authored_geyser_count': 0,
                        'geyser_policy': 'source neutral placements retained',
                        'geysers': geysers,
                        'cells': len(tile_ids), 'unique_runtime_tiles': len(pixels),
                        'blocked_cells': sum(v & 1 for v in blocked),
                        'chk_sha1': hashlib.sha1(chk).hexdigest(),
                        'terrain_assets_sha256': {k: hashlib.sha256(v).hexdigest() for k, v in sources.items()}})
    lines.extend([f'static const Sc64SkirmishMap sc64_skirmish_maps[{len(MAPS)}] = {{\n', *descriptors, '};\n', '#endif /* SC64_SKIRMISH_GENERATED_H */\n'])
    content = ''.join(lines)
    output.parent.mkdir(parents=True, exist_ok=True)
    if include_cover: content='#define SC64_SKIRMISH_COVER_DATA 1\n'+content
    output.write_text(content, encoding='ascii')
    report = {'schema': 1, 'source': {'rom_sha1': ROM_SHA1,
              'extractor': 'references/sc64-maps/extract_sc64_maps.py', 'extractor_commit': UPSTREAM_COMMIT},
              'maps': reports, 'header': str(output),
              'header_sha256': hashlib.sha256(content.encode('ascii')).hexdigest(),
              'palette_policy': 'nearest used world index in fixed Backwater RGB555 palette',
              'collision_policy': 'VF4 central four minitiles required walkable; four-connected flood fill validates all selected starts'}
    output.with_name('sc64_skirmish_pack_report.json').write_text(json.dumps(report, indent=2) + '\n')
    return report


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rom', type=Path, default=ROM)
    parser.add_argument('--backwater-generated', type=Path, default=BASE)
    parser.add_argument('--out', type=Path, default=OUTPUT)
    parser.add_argument('--cover', action='store_true')
    args = parser.parse_args()
    try:
        print(json.dumps(pack(args.rom, args.backwater_generated, args.out, args.cover), indent=2))
    except (OSError, ValueError, RuntimeError, KeyError) as exc:
        print(f'sc64_skirmish_pack: {exc}', file=sys.stderr)
        raise SystemExit(1)
