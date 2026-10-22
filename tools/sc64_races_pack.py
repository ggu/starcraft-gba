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
    # T3 is emitted into the private header, but its data and map count are
    # compiled only by unified builds that explicitly enable stage 3.
    ('T3', '008/00C', 'Desperate Alliance'),
    # Z3 has its fixed global catalog ID 14. Unified builds keep catalog
    # slots 7..13 explicitly unavailable rather than compacting this index.
    ('Z3', '008/016', 'The New Dominion'),
]
BUILDING_RAW_IDS = {
    106, 109, 110, 111, 112, 113, 114, 122, 124, 125, 130, 131, 132,
    135, 139, 141, 142, 143, 144, 146, 149, 151, 154, 155, 156, 157,
    160, 162, 164, 166, 167,
}


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


def emit_map_count() -> str:
    """Emit compile-time map count for the stable direct-global-ID table."""
    return (
        '#if defined(SC64_UNIFIED) && defined(SC64_Z3_ENABLED)\n'
        '#define SC64_RACE_MAP_COUNT 15\n'
        '#elif defined(SC64_UNIFIED) && defined(SC64_T3_ENABLED)\n'
        '#define SC64_RACE_MAP_COUNT 7\n'
        '#elif defined(SC64_UNIFIED)\n'
        '#define SC64_RACE_MAP_COUNT 6\n'
        '#else\n#define SC64_RACE_MAP_COUNT 4\n#endif\n'
    )


def campaign_guard_for_map(map_index: int) -> str:
    """Return the profile guard used around one map's data and descriptor."""
    if map_index < 2 or map_index == 7:
        race = 1
    elif map_index < 4:
        race = 2
    else:
        race = 0
    return f'!defined(SC64_CAMPAIGN_RACE) || SC64_CAMPAIGN_RACE == {race}'


def emit_descriptor_table(descriptors: list[str]) -> str:
    """Emit stable direct-ID descriptor slots, with disabled IDs zeroed.

    ``descriptors`` uses packer order: four race missions, T1, T2, T3, Z3.
    The per-descriptor race guards are carried inside each supplied token.
    """
    if len(descriptors) != 8:
        raise ValueError('expected descriptors for map indices 0..7')
    return ''.join([
        'static const Sc64RaceMap sc64_race_maps[SC64_RACE_MAP_COUNT] = {\n',
        *descriptors[:4],
        '#ifdef SC64_UNIFIED\n', *descriptors[4:6], '#endif /* SC64_UNIFIED */\n',
        '#if defined(SC64_UNIFIED)\n',
        '#ifdef SC64_T3_ENABLED\n', descriptors[6],
        '#elif defined(SC64_Z3_ENABLED)\n    {0}, /* global ID 6 is unavailable without T3 */\n',
        '#endif\n',
        '#if defined(SC64_Z3_ENABLED)\n',
        '    {0}, /* global ID 7 is unavailable */\n'
        '    {0}, /* global ID 8 is unavailable */\n'
        '    {0}, /* global ID 9 is unavailable */\n'
        '    {0}, /* global ID 10 is unavailable */\n'
        '    {0}, /* global ID 11 is unavailable */\n'
        '    {0}, /* global ID 12 is unavailable */\n'
        '    {0}, /* global ID 13 is unavailable */\n',
        descriptors[7], '#endif /* SC64_Z3_ENABLED */\n',
        '#endif /* SC64_UNIFIED */\n',
        '};\n',
    ])


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
        emit_map_count(),
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
        force_data = sections.get(b'FORC', b'')
        force_assignments = list(force_data[:8]) if len(force_data) >= 8 else []
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
        elif map_index == 6:
            lines.append('#if defined(SC64_UNIFIED) && defined(SC64_T3_ENABLED)\n')
        elif map_index == 7:
            lines.append('#if defined(SC64_UNIFIED) && defined(SC64_Z3_ENABLED)\n')
        campaign_guard = campaign_guard_for_map(map_index)
        lines.append(f'#if {campaign_guard}\n')
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
        lines.append('#endif /* selected campaign assets */\n')
        if map_index == 5:
            lines.append('#endif /* SC64_UNIFIED */\n')
        elif map_index == 6:
            lines.append('#endif /* SC64_UNIFIED && SC64_T3_ENABLED */\n')
        elif map_index == 7:
            lines.append('#endif /* SC64_UNIFIED && SC64_Z3_ENABLED */\n')
        descriptors.append(f'#if {campaign_guard}\n'
            f'    {{{info.width}u,{info.height}u,{tileset}u,{human_slot}u,sc64_race_{name}_owner_modes,sc64_race_{name}_owner_races,{unit_name},{len(units)}u,sc64_race_{name}_tiles,{len(tiles)}u,sc64_race_{name}_lookup,sc64_race_{name}_blocked,"{info.name}","{mission_id}"}},\n'
            '#else\n    {0}, /* campaign omitted from this cartridge */\n#endif\n')
        triggers = importer.trigger_rows(sections.get(b'TRIG', b''), sections.get(b'STR ', b''), x)
        objectives = [a.get('text') for tr in triggers for a in tr['actions'] if a['opcode'] == 12 and a.get('text')]
        reports.append({
            'mission_id': mission_id, 'path': path, 'title': info.name,
            'dimensions': [info.width, info.height], 'tileset_id': tileset,
            'human_slot': human_slot, 'human_side': sides[human_slot],
            'owner_modes': owner_modes,
            'owner_races': owner_races,
            'force_assignments_raw': force_assignments,
            'source_sides': source_sides,
            'chk_sha1': hashlib.sha1(chk).hexdigest(), 'unit_records': len(units),
            'trigger_records': len(triggers), 'objective_text': objectives,
            'unique_source_tiles': len(decoded), 'unique_runtime_tiles': len(tiles),
            'cells': len(tile_ids), 'blocked_cells': sum(blocked),
            'terrain_assets_sha256': {k: hashlib.sha256(v).hexdigest() for k, v in sources.items()},
        })
        if map_index >= 4:
            source_units = importer.unit_rows(sections.get(b'UNIT', b''))
            source_categories = Counter(
                'start_marker' if u['source_type'] == 214 else
                'mineral' if u['source_type'] in (176, 177, 178) else
                'geyser' if u['source_type'] == 188 else
                'building' if u['source_type'] in BUILDING_RAW_IDS else
                'unit'
                for u in source_units
            )
            reports[-1]['source_unit_records'] = len(source_units)
            reports[-1]['runtime_unit_records'] = len(units)
            reports[-1]['excluded_start_markers'] = source_categories['start_marker']
            reports[-1]['source_categories'] = {
                'unit_placements': source_categories['unit'],
                'building_placements': source_categories['building'],
                'mineral_fields': source_categories['mineral'],
                'geysers': source_categories['geyser'],
            }
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
            if map_index == 7:
                expected = {'unit_placements': 59, 'building_placements': 32,
                            'mineral_fields': 28, 'geysers': 3}
                category_keys = {
                    'unit_placements': 'unit', 'building_placements': 'building',
                    'mineral_fields': 'mineral', 'geysers': 'geyser',
                }
                actual = {key: source_categories[source_key]
                          for key, source_key in category_keys.items()}
                if actual != expected or source_categories['start_marker'] != 2:
                    raise ValueError(f'Z3 source UNIT roster changed: {actual}, '
                                     f"start markers={source_categories['start_marker']}")
                if len(units) != sum(expected.values()):
                    raise ValueError('Z3 importer skipped or duplicated non-start UNIT rows')
                if len(force_assignments) != 8 or force_assignments[3] != 0 or force_assignments[6] != 1:
                    raise ValueError('Z3 human/enemy force assignment no longer matches audited CHK')
                reports[-1]['global_mission_id'] = 14
                reports[-1]['force_assignment_summary'] = {
                    'human_owner_slot_3': force_assignments[3],
                    'enemy_owner_slot_6': force_assignments[6],
                    'interpretation': 'Force 1 and Force 2 respectively; raw FORCE section bytes are preserved above',
                }
                reports[-1]['runtime_unit_rows_are_all_non_start_source_rows'] = True
    lines.extend([emit_descriptor_table(descriptors), '#endif\n'])
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
