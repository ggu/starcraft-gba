#!/usr/bin/env python3
"""Generate private Zerg and Protoss unit sprites from a verified SC64 ROM.

The generated C header and rendered PNG previews contain extracted game art
and must remain local under private.local/. No ROM data is committed.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from tools import sc64_sprites as base  # noqa: E402

ROM = ROOT / '.local/sc64/rom/StarCraft64-USA.z64'
PALETTE_REMAP = ROOT / '.local/sc64/backwater/generated/palette-remap.bin'
OUT = ROOT / '.local/sc64/races/generated/sprites'
HEADER = ROOT / '.local/sc64/races/generated/sc64_race_sprites_generated.h'

# Brood War raw unit ids. Entries cover actual map records and the normal
# skirmish build menu. Names are stable C identifiers used by the game.
RACE_UNIT_IDS = {
    'marine': 0, 'ghost': 1, 'vulture': 2, 'goliath': 3, 'siege_tank': 5,
    'scv': 7, 'wraith': 8, 'science_vessel': 9, 'dropship': 11,
    'firebat': 32,
    'battlecruiser': 12, 'hero_fenix': 77, 'fenix': 77,
    'tassadar': 78,
    'probe': 64, 'zealot': 65, 'dragoon': 66, 'high_templar': 67,
    'archon': 68, 'shuttle': 69, 'scout': 70, 'carrier': 72,
    'interceptor': 73, 'reaver': 83, 'observer': 84,
    'zergling': 37, 'hydralisk': 38, 'ultralisk': 39, 'broodling': 40,
    'drone': 41, 'overlord': 42, 'mutalisk': 43, 'guardian': 44,
    'queen': 45, 'scourge': 46, 'defiler': 47, 'larva': 35, 'egg': 36,
    'lurker': 103,
    'hatchery': 131, 'lair': 132, 'hive': 133, 'engineering_bay': 122,
    'hydralisk_den': 135,
    'defiler_mound': 136, 'greater_spire': 137, 'queens_nest': 138,
    'evolution_chamber': 139, 'ultralisk_cavern': 140, 'spire': 141,
    'spawning_pool': 142, 'creep_colony': 143, 'spore_colony': 144,
    'sunken_colony': 146, 'overmind_with_shell': 147, 'overmind': 148,
    'extractor': 149, 'mature_chrysalis': 150, 'cerebrate': 151,
    'cerebrate_daggoth': 152,
    'nexus': 154, 'robotics_facility': 155, 'pylon': 156,
    'assimilator': 157, 'observatory': 159, 'gateway': 160,
    'photon_cannon': 162, 'citadel_of_adun': 163, 'cybernetics_core': 164,
    'templar_archives': 165, 'forge': 166, 'stargate': 167,
    'stasis_cell_prison': 168, 'fleet_beacon': 169,
    'command_center': 106, 'supply_depot': 109, 'barracks': 111,
    'academy': 112, 'starport': 114, 'missile_turret': 124, 'bunker': 125,
    'infested_command_center': 130, 'kakaru': 53, 'young_chrysalis': 216,
    'mineral_field_1': 176, 'mineral_field_2': 177, 'mineral_field_3': 178,
    'geyser': 188, 'special_zerg_beacon': 194,
}

# These types occur in the four authored campaign scenarios. Their game names
# are scenario-specific or decorative; preserve actual indexed art for them
# under an explicit source-id label rather than drawing a substitute.
MISSION_RAW_TYPES = (0, 3, 7, 8, 32, 37, 38, 41, 42, 43, 53, 64, 65, 66,
                     70, 77, 106, 109, 111, 112, 114, 124, 125, 130, 131,
                     132, 135, 139, 141, 142, 143, 144, 146, 149, 151, 154,
                     156, 157, 160, 162, 164, 166, 167, 176, 177, 178, 188,
                     194, 216)
for _raw in MISSION_RAW_TYPES:
    if _raw not in RACE_UNIT_IDS.values():
        RACE_UNIT_IDS[f'raw_unit_{_raw}'] = _raw


def write_header(path: Path, selected: list[dict], remap: bytes) -> None:
    lines = [
        '/* Generated from the verified USA ROM; private local asset. */',
        '#ifndef SC64_RACE_SPRITES_GENERATED_H',
        '#define SC64_RACE_SPRITES_GENERATED_H',
        '#ifndef SC64_SPRITES_GENERATED_H',
        'typedef struct { short offset_x, offset_y; unsigned short w, h, canvas_w, canvas_h; const unsigned char *pixels; } Sc64SpriteFrame;',
        '#endif',
    ]
    for item in selected:
        name, frame = item['name'], item['frame']
        data = bytearray(base.crop_frame(frame))
        x, y, fw = frame['x'], frame['y'], frame['frame_width']
        for row in range(frame['frame_height']):
            for col in range(fw):
                si = (y + row) * frame['width'] + x + col
                di = row * fw + col
                data[di] = remap[data[di]] if frame['alpha'][si] else 0
        symbol = f'sc64_race_sprite_{name}'
        lines.append(f'static const unsigned char {symbol}_pixels[{len(data)}] = {{')
        for i in range(0, len(data), 16):
            lines.append('    ' + ','.join(f'0x{v:02x}' for v in data[i:i + 16]) + ',')
        lines += ['};',
                  f'static const Sc64SpriteFrame {symbol} = '
                  f'{{{frame["x"]},{frame["y"]},{fw},{frame["frame_height"]},'
                  f'{frame["width"]},{frame["height"]},{symbol}_pixels}};']
    lines += ['static inline const Sc64SpriteFrame *sc64_race_unit_sprite(unsigned short raw_type) {',
              '    switch (raw_type) {']
    seen = set()
    for name, raw_type in RACE_UNIT_IDS.items():
        if raw_type in seen:
            continue
        seen.add(raw_type)
        lines.append(f'    case {raw_type}: return &sc64_race_sprite_{name};')
    lines += ['    default: return (const Sc64SpriteFrame *)0;', '    }', '}', '#endif']
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text('\n'.join(lines) + '\n', encoding='ascii')


def generate(rom: Path = ROM, out: Path = OUT, header: Path = HEADER,
             remap_path: Path = PALETTE_REMAP) -> dict:
    raw = rom.read_bytes()
    extractor = base.load_extractor(base.UPSTREAM)
    normalized = extractor.load_rom(str(rom))
    sha1 = hashlib.sha1(normalized).hexdigest()
    if sha1 != base.EXPECTED_USA_ROM_SHA1:
        raise ValueError(f'Expected verified USA ROM {base.EXPECTED_USA_ROM_SHA1}, got {sha1}')
    remap = remap_path.read_bytes()
    if len(remap) != 256:
        raise ValueError('Backwater sprite remap must have 256 entries')
    archive = extractor.BoltArchive(normalized)
    refs = base.resolve_grp_indices(archive, RACE_UNIT_IDS)
    palette = base.decode_rgba5551_palette(base._entry_payload(archive, '00B/004'))
    entries = {entry.path: entry for entry in archive.entries()}
    selected, manifest_assets = [], []
    out.mkdir(parents=True, exist_ok=True)
    for name, ref in refs.items():
        grp_data = archive.read(entries[ref['bolt_path']])
        grp = base.parse_grp(grp_data)
        frame = grp['frames'][0]
        png = out / f'{name}.png'
        base.encode_png(png, grp['width'], grp['height'], base.render_frame(frame, palette))
        selected.append({'name': name, 'frame': frame})
        manifest_assets.append({'name': name, **ref, 'sha1': hashlib.sha1(grp_data).hexdigest(),
                                'frame_count': len(grp['frames']),
                                'canvas': [grp['width'], grp['height']],
                                'selected_frame': 0, 'frame_rect': [frame['x'], frame['y'],
                                frame['frame_width'], frame['frame_height']],
                                'opaque_pixels': sum(frame['alpha']), 'preview': png.name})
    write_header(header, selected, remap)
    report = {'source': {'rom_sha1': sha1, 'rom_input_sha1': hashlib.sha1(raw).hexdigest(),
                         'upstream_commit': base.UPSTREAM_COMMIT},
              'palette': 'Shared Backwater GBA sprite palette; map terrain colors are quantized offline to avoid runtime sprite recoloring.',
              'raw_unit_ids': RACE_UNIT_IDS, 'assets': manifest_assets,
              'generated_header': str(header)}
    (out / 'manifest.json').write_text(json.dumps(report, indent=2) + '\n')
    return report


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rom', type=Path, default=ROM)
    parser.add_argument('--out', type=Path, default=OUT)
    parser.add_argument('--header', type=Path, default=HEADER)
    parser.add_argument('--remap', type=Path, default=PALETTE_REMAP)
    args = parser.parse_args()
    try:
        report = generate(args.rom, args.out, args.header, args.remap)
    except (OSError, ValueError, RuntimeError, KeyError) as exc:
        print(f'sc64_race_sprites: {exc}', file=sys.stderr)
        return 1
    print(f"Generated {len(report['assets'])} race sprites into {args.header}")
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
