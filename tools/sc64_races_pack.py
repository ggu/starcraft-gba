#!/usr/bin/env python3
"""Generate private terrain and placement tables for the first Zerg/Protoss missions.

All extracted game data and generated headers stay under ignored .local/.
The header is derived from a user-supplied verified StarCraft 64 USA ROM and
the pinned sc64-maps extractor; it must not be committed or distributed.
"""
from __future__ import annotations

from collections import Counter
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
ROM = ROOT / '.local/sc64/rom/StarCraft64-USA.z64'
BASE = ROOT / '.local/sc64/backwater/generated'
OUTPUT = ROOT / '.local/sc64/races/generated/sc64_races_generated.h'
ROM_SHA1 = 'a36d91b5aac7bb95df0d7de5f9814f5270e25c2b'
UPSTREAM_COMMIT = '09b57f787a247107d1de82b8c5eab2734667ea1d'
MISSIONS = [
    ('Z1', '008/014', 'Among the Ruins'),
    ('Z2', '008/015', 'Egression'),
    ('P1', '008/01E', 'First Strike'),
    ('P2', '008/01F', 'Into the Flames'),
    # Keep these after the four Races missions: save slots and current builds
    # depend on the original indices remaining Z1/Z2/P1/P2 = 0..3.
    ('T1', '008/00A', 'Wasteland'),
    ('T2', '008/00B', 'Backwater Station'),
]


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


def pack(rom_path: Path = ROM, base: Path = BASE, output: Path = OUTPUT) -> dict:
    importer = load('sc64_races_import', ROOT / 'tools/sc64_import.py')
    assets = load('sc64_races_assets', ROOT / 'tools/sc64_assets.py')
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

    backwater_palette = json.loads((base / 'palette.json').read_text())['gba_rgb555']
    remap_data = (base / 'palette-remap.bin').read_bytes()
    if len(backwater_palette) != 256 or len(remap_data) != 256:
        raise ValueError('Prepare Backwater generated palette/remap before packing race maps')
    shared_indices = sorted({i for i in remap_data if i >= 32})
    if not shared_indices:
        raise ValueError('Backwater sprite palette profile has no world colors')

    maps = list(x.find_maps(archive))
    lines = [
        '/* Private generated data. Do not commit or distribute. */\n',
        '/* Source: references/sc64-maps/extract_sc64_maps.py @ ' + UPSTREAM_COMMIT + ' */\n',
        '#ifndef SC64_RACES_GENERATED_H\n#define SC64_RACES_GENERATED_H\n',
        '#include "sc64_types.h"\n',
        '#ifdef SC64_UNIFIED\n#define SC64_RACE_MAP_COUNT 6\n'
        '#else\n#define SC64_RACE_MAP_COUNT 4\n#endif\n',
        'typedef struct {\n'
        '    unsigned short width, height;\n'
        '    unsigned char tileset_id, human_slot;\n'
        '    const unsigned char *owner_modes, *owner_races;\n'
        '    const Sc64ImportedUnit *units; unsigned short unit_count;\n'
        '    const unsigned char (*tile_pixels)[256]; unsigned short tile_count;\n'
        '    const unsigned short *tile_lookup; const unsigned char *blocked;\n'
        '    const char *title; const char *mission_id;\n'
        '} Sc64RaceMap;\n',
    ]
    reports = []
    descriptors = []
    for map_index, (mission_id, path, _title) in enumerate(MISSIONS):
        selected = next(((i, chk) for i, chk in maps if i.bolt_path == path), None)
        if selected is None:
            raise ValueError(f'Mission {path} is absent from the verified ROM')
        info, chk = selected
        sections = importer.sections(chk, x)
        owners = sections.get(b'OWNR', b'')
        sides = sections.get(b'SIDE', b'')
        human_slots = [slot for slot, owner in enumerate(owners) if owner == 6]
        if len(human_slots) != 1 or len(sides) < len(owners):
            raise ValueError(f'{path} has no unique human OWNR/SIDE slot')
        human_slot = human_slots[0]
        owner_modes = [2 if owner == 11 else 3 if owner == 3 else
                       0 if owner == 6 else 1 if owner in (1, 5, 7) else 2
                       for owner in owners[:8]]
        # CHK SIDE uses Terran=1, Zerg=0, Protoss=2; normalize into the
        # compact runtime enum (R_TER=0, R_ZERG=1, R_PROTOSS=2).
        source_sides = list(sides[:8])
        owner_races = [0 if side == 1 else 1 if side == 0 else 2 if side == 2 else 3
                       for side in source_sides]
        raw = sections.get(b'MTXM', b'')
        if len(raw) != info.width * info.height * 2:
            raise ValueError(f'{path} MTXM dimensions do not match')
        tile_ids = list(struct.unpack(f'<{len(raw) // 2}H', raw))
        tileset = info.tileset & 7
        # BOLT/00B stores eight consecutive 15-entry graphics groups, one per
        # StarCraft tileset. Each group's first five files are VR4/VX4/VF4/CV5/WPE.
        asset_base = tileset * 0x0f
        sources = {label: read(f'00B/{asset_base + index:03X}') for label, index in
                   [('vr4', 0), ('vx4', 1), ('vf4', 2), ('cv5', 3), ('palette', 4)]}
        decoded = {t: assets.decode_tile(t, sources['cv5'], sources['vx4'],
                                         sources['vf4'], sources['vr4']) for t in set(tile_ids)}
        source_palette = assets.palette_rgb555(sources['palette'])
        # Terrain pixels and sprites share the existing Backwater palette.
        # Quantize every source terrain color offline; runtime rendering then
        # uses the same fixed palette with no per-map sprite recoloring.
        palette_remap = bytes(nearest(color, backwater_palette, shared_indices)
                              for color in source_palette)
        tiles: list[bytes] = []
        lookup: list[int] = []
        blocked: list[int] = []
        dedup: dict[bytes, int] = {}
        for tile_id in tile_ids:
            pixels, walk = decoded[tile_id]
            packed = pixels.translate(palette_remap)
            if packed not in dedup:
                dedup[packed] = len(tiles)
                tiles.append(packed)
            lookup.append(dedup[packed])
            blocked.append(int((walk & 0x0660) != 0x0660))
        units = [u for u in importer.unit_rows(sections.get(b'UNIT', b'')) if u['source_type'] != 214]
        name = mission_id.lower()
        # The race-only binary and tests must not acquire the much larger
        # Terran campaign payload. Unified builds opt into both map datasets.
        if map_index == 4:
            lines.append('#ifdef SC64_UNIFIED\n')
        unit_name = f'sc64_race_{name}_units'
        lines.append(f'static const Sc64ImportedUnit {unit_name}[{len(units)}] = {{\n')
        for unit in units:
            if unit['flags'] > 0xffff or unit['resources'] > 0xffff:
                raise ValueError(f'{path} unit fields exceed runtime schema')
            lines.append(f"    {{{unit['x_source_px']//2}u,{unit['y_source_px']//2}u,{unit['source_type']}u,{unit['owner_raw']}u,{unit['hp_percent']}u,{unit['flags']}u,{unit['resources']}u}},\n")
        lines.append('};\n')
        lines.append(c_array(f'sc64_race_{name}_owner_modes', 'unsigned char', owner_modes, 8))
        lines.append(c_array(f'sc64_race_{name}_owner_races', 'unsigned char', owner_races, 8))
        lines.append(c_array(f'sc64_race_{name}_lookup', 'unsigned short', lookup))
        lines.append(c_array(f'sc64_race_{name}_blocked', 'unsigned char', blocked))
        lines.append(f'static const unsigned char sc64_race_{name}_tiles[{len(tiles)}][256] __attribute__((aligned(4))) = {{\n')
        lines.extend('{' + ','.join(map(str, tile)) + '},\n' for tile in tiles)
        lines.append('};\n')
        if map_index == 5:
            lines.append('#endif /* SC64_UNIFIED */\n')
        descriptors.append(f'    {{{info.width}u,{info.height}u,{tileset}u,{human_slot}u,sc64_race_{name}_owner_modes,sc64_race_{name}_owner_races,{unit_name},{len(units)}u,sc64_race_{name}_tiles,{len(tiles)}u,sc64_race_{name}_lookup,sc64_race_{name}_blocked,"{info.name}","{mission_id}"}},\n')
        triggers = importer.trigger_rows(sections.get(b'TRIG', b''), sections.get(b'STR ', b''), x)
        objectives = [a.get('text') for tr in triggers for a in tr['actions'] if a['opcode'] == 12 and a.get('text')]
        reports.append({
            'mission_id': mission_id, 'path': path, 'title': info.name,
            'dimensions': [info.width, info.height], 'tileset_id': tileset,
            'human_slot': human_slot, 'human_side': sides[human_slot],
            'owner_modes': owner_modes,
            'owner_races': owner_races,
            'source_sides': source_sides,
            'chk_sha1': hashlib.sha1(chk).hexdigest(), 'unit_records': len(units),
            'trigger_records': len(triggers), 'objective_text': objectives,
            'unique_source_tiles': len(decoded), 'unique_runtime_tiles': len(tiles),
            'cells': len(tile_ids), 'blocked_cells': sum(blocked),
            'terrain_assets_sha256': {k: hashlib.sha256(v).hexdigest() for k, v in sources.items()},
        })
        if map_index >= 4:
            reports[-1]['runtime_unit_locations'] = [
                {'source_type': u['source_type'], 'owner_raw': u['owner_raw'],
                 'x': u['x_source_px'] // 2, 'y': u['y_source_px'] // 2,
                 'hp_percent': u['hp_percent'], 'flags': u['flags'],
                 'resources': u['resources']}
                for u in units
            ]
            reports[-1]['rescuable_units'] = [
                {'source_type': u['source_type'], 'owner_raw': u['owner_raw'],
                 'x': u['x_source_px'] // 2, 'y': u['y_source_px'] // 2}
                for u in units if u['owner_raw'] == 3
            ]
    lines.extend([
        'static const Sc64RaceMap sc64_race_maps[SC64_RACE_MAP_COUNT] = {\n',
        *descriptors[:4],
        '#ifdef SC64_UNIFIED\n', *descriptors[4:], '#endif /* SC64_UNIFIED */\n',
        '};\n', '#endif\n',
    ])
    content = ''.join(lines)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(content, encoding='ascii')
    report = {
        'schema': 1,
        'source': {'rom_sha1': ROM_SHA1, 'extractor': 'references/sc64-maps/extract_sc64_maps.py', 'extractor_commit': UPSTREAM_COMMIT},
        'maps': [{'index': index, **row} for index, row in enumerate(reports)],
        'header': str(output), 'header_sha256': hashlib.sha256(content.encode('ascii')).hexdigest(),
        'palette_policy': 'all terrain colors are quantized offline to nearest used world color in the fixed Backwater palette; source indices remap directly to shared palette indices 32..255, preserving sprites without runtime recoloring',
        'collision_policy': 'VF4 walk bit 0; central four minitiles required walkable; this is a coarse 16px GBA pathfinding approximation',
    }
    output.with_name('sc64_races_pack_report.json').write_text(json.dumps(report, indent=2) + '\n')
    return report


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rom', type=Path, default=ROM)
    parser.add_argument('--backwater-generated', type=Path, default=BASE)
    parser.add_argument('--out', type=Path, default=OUTPUT)
    args = parser.parse_args()
    try:
        print(json.dumps(pack(args.rom, args.backwater_generated, args.out), indent=2))
    except (OSError, ValueError, RuntimeError, KeyError) as exc:
        print(f'sc64_races_pack: {exc}', file=sys.stderr)
        raise SystemExit(1)
