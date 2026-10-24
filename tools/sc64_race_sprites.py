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
    'queen': 45, 'defiler': 46, 'scourge': 47, 'larva': 35, 'egg': 36,
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

# Additional source-backed IDs needed by The Jacobs Installation (T4). Keep
# this set out of the shared race sprite list so non-T4 profiles retain their
# exact selected art and switch cases. Raw 101 is deliberately not an image:
# it is a map-revealer actor and must remain invisible in the source behavior.
T4_UNIT_IDS = {
    'civilian': 15,
    'raynor_raw20': 20,
    'rhynadon': 89,
    'ragnasaur': 95,
    'terran_beacon': 195,
    'data_disk': 218,
}
# Source-backed map prop GRPs required by Jacobs Installation. Preserve every
# indexed source frame; no frame number is interpreted as an open/closed state.
T4_BUILDING_IDS = {
    'floor_missile_trap': 203, 'upper_door': 205,
    'right_upper_door': 206, 'pit_door': 207, 'right_pit_door': 208,
    'floor_gun_trap': 209, 'wall_flame_trap': 211,
    'right_wall_missile_trap': 212,
}
T4_INVISIBLE_RAW_TYPES = (101,)
T4_EXPECTED_VISIBLE_RAW_TYPES = (0, 1, 2, 3, 7, 15, 20, 32, 37, 89, 95, 195, 218)
# The crashed objective is not raw-29 Hero_Norad_II. Raw-19 Raynor and the
# Control Tower also need their own art for T6's hero/transport route.
# Queen's Nest (138) already has a shared source sprite above.
T6_UNIT_IDS = {'crashed_norad_ii': 126, 'raynor_vulture_t6': 19,
               'control_tower_t6': 115}

# Distinct late Terran campaign actors not already covered by the shared race,
# T4, or T6 sets. These are private source GRPs behind the late-runtime flag.
LATE_UNIT_IDS = {'norad_ii': 29, 'psi_emitter': 217}
LATE_BUILDING_IDS = {
    'nuclear_silo': 108, 'covert_ops': 117, 'machine_shop': 120,
    'armory_t8': 123, 'ion_cannon': 127,
    'robotics_support_bay': 171, 'shield_battery': 172,
    'science_facility': 116, 'physics_lab': 118,
}
# Authenticated scenario actors that are not part of the standard roster.
# Raw 101 is intentionally represented as invisible mode, never extracted art.
ZP_UNIT_IDS = {'infested_kerrigan': 51, 'raynor_vulture_zp': 19,
               'tassadar_hero_zp': 79}
ZP_BUILDING_IDS = {'control_tower_zp': 115, 'machine_shop_zp': 120,
                   'armory_zp': 123, 'queens_nest_zp': 138,
                   'ultralisk_cavern_zp': 140, 'protoss_beacon_zp': 196}
ZP_INVISIBLE_RAW_TYPES = (101,)

# Z5-only completeness set. Existing base/T4 assets may contain some of these
# raw IDs, but this is a separately gated, separately validated source set.
Z5_UNIT_IDS = {
    'ghost_z5': 1, 'civilian_z5': 15, 'rhynadon_z5': 89,
    'bengalaas_z5': 90, 'ragnasaur_z5': 95, 'data_disk_z5': 218,
}
Z5_BUILDING_IDS = {'terran_beacon_z5': 195}
Z5_DOODAD_IDS = {
    'floor_missile_trap_z5': 203, 'upper_door_z5': 205,
    'right_upper_door_z5': 206, 'pit_door_z5': 207,
    'right_pit_door_z5': 208, 'floor_gun_trap_z5': 209,
    'wall_flame_trap_z5': 211, 'right_wall_flame_trap_z5': 213,
}

# Guardian is listed in the ordinary sprite roster for compatibility, while
# future Z6 adapters also require an independently complete/gated payload.
Z6_UNIT_IDS = {'guardian_z6': 44}
Z6_BUILDING_IDS = {'robotics_support_bay_z6': 171, 'shield_battery_z6': 172}

# P6-specific actor GRPs. The raw 1 Ghost, beacons, doors, and the existing
# floor/wall traps are supplied by the base/T4/ZP/Z5 groups; only these three
# actor images and raw 210's indexed wall-trap prop GRP are new here.
P6_UNIT_IDS = {
    'infested_terran_p6': 50,
    'hero_dark_templar_p6': 74,
    'zeratul_p6': 75,
}
P6_DOODAD_IDS = {'wall_missile_trap_p6': 210}

# The shared raw 68/72/84 unit art already comes from RACE_UNIT_IDS. P7 adds
# Arbiter art plus complete frame banks for all seven Protoss structures.
P7_UNIT_IDS = {'arbiter_p7': 71}
P7_BUILDING_IDS = {
    'observatory_p7': 159, 'citadel_of_adun_p7': 163,
    'templar_archives_p7': 165, 'fleet_beacon_p7': 169,
    'arbiter_tribunal_p7': 170, 'robotics_support_bay_p7': 171,
    'shield_battery_p7': 172,
}
P7_SHARED_UNIT_IDS = {'archon': 68, 'carrier': 72, 'observer': 84}


def asset_manifest_sha256(assets: list[dict]) -> str:
    """Hash stable source-asset identity fields without preview/path noise."""
    canonical = [{key: asset[key] for key in
                  ('name', 'unit_id', 'flingy_id', 'sprite_id', 'image_id',
                   'grp_filename_index', 'bolt_path', 'sha1', 'canvas',
                   'selected_frame', 'frame_rect', 'opaque_pixels')}
                 for asset in assets]
    payload = json.dumps(canonical, sort_keys=True, separators=(',', ':')).encode('ascii')
    return hashlib.sha256(payload).hexdigest()


def named_asset_manifest_sha256(assets: list[dict], names: set[str]) -> str:
    """Hash only the explicitly named asset group, preserving manifest order."""
    selected = [asset for asset in assets if asset.get('name') in names]
    return asset_manifest_sha256(selected)

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


def write_header(path: Path, selected: list[dict], remap: bytes,
                 t4_selected: list[dict] | None = None,
                 t4_building_selected: list[dict] | None = None,
                 t6_selected: list[dict] | None = None,
                 late_selected: list[dict] | None = None,
                 late_building_selected: list[dict] | None = None,
                 zp_selected: list[dict] | None = None,
                 zp_building_selected: list[dict] | None = None,
                 z5_selected: list[dict] | None = None,
                 z5_building_selected: list[dict] | None = None,
                 z6_selected: list[dict] | None = None,
                 z5_doodad_selected: list[dict] | None = None,
                 p6_selected: list[dict] | None = None,
                 p6_doodad_selected: list[dict] | None = None,
                 z6_building_selected: list[dict] | None = None,
                 p7_selected: list[dict] | None = None,
                 p7_building_selected: list[dict] | None = None) -> None:
    lines = [
        '/* Generated from the verified USA ROM; private local asset. */',
        '#ifndef SC64_RACE_SPRITES_GENERATED_H',
        '#define SC64_RACE_SPRITES_GENERATED_H',
        '#ifndef SC64_SPRITES_GENERATED_H',
        'typedef struct { short offset_x, offset_y; unsigned short w, h, canvas_w, canvas_h; const unsigned char *pixels; } Sc64SpriteFrame;',
        '#endif',
    ]
    def append_asset(item: dict) -> None:
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
        lines.extend(['};',
                      f'static const Sc64SpriteFrame {symbol} = '
                      f'{{{frame["x"]},{frame["y"]},{fw},{frame["frame_height"]},'
                      f'{frame["width"]},{frame["height"]},{symbol}_pixels}};'])
    for item in selected:
        append_asset(item)
    if t4_selected or t4_building_selected:
        lines.append('#ifdef SC64_T4_ENABLED')
        for item in (t4_selected or []):
            append_asset(item)
        for item in (t4_building_selected or []):
            for frame_index, frame in enumerate(item['frames']):
                append_asset({'name': f'{item["name"]}_frame_{frame_index}', 'frame': frame})
        lines.append('#endif /* SC64_T4_ENABLED */')
    if t6_selected:
        lines.append('#ifdef SC64_T6_ENABLED')
        if {item['name'] for item in t6_selected} == set(T6_UNIT_IDS):
            lines.append('#define SC64_T6_SPRITE_SET_COMPLETE 1')
        for item in t6_selected:
            append_asset(item)
        lines.append('#endif /* SC64_T6_ENABLED */')
    if late_selected or late_building_selected:
        lines.append('#ifdef SC64_TERRAN_LATE_ENABLED')
        if ({item['name'] for item in (late_selected or [])} == set(LATE_UNIT_IDS) and
                {item['name'] for item in (late_building_selected or [])} ==
                set(LATE_BUILDING_IDS)):
            lines.append('#define SC64_TERRAN_LATE_SPRITE_SET_COMPLETE 1')
        for item in (late_selected or []):
            append_asset(item)
        for item in (late_building_selected or []):
            for frame_index, frame in enumerate(item['frames']):
                append_asset({'name': f'{item["name"]}_frame_{frame_index}', 'frame': frame})
        lines.append('#endif /* SC64_TERRAN_LATE_ENABLED */')
    if zp_selected or zp_building_selected:
        lines.append('#ifdef SC64_ZP_CAMPAIGN_ACTORS')
        if ({item['name'] for item in (zp_selected or [])} == set(ZP_UNIT_IDS) and
                {item['name'] for item in (zp_building_selected or [])} == set(ZP_BUILDING_IDS)):
            lines.append('#define SC64_ZP_CAMPAIGN_SPRITE_SET_COMPLETE 1')
        for item in (zp_selected or []):
            append_asset(item)
        for item in (zp_building_selected or []):
            for frame_index, frame in enumerate(item['frames']):
                append_asset({'name': f'{item["name"]}_frame_{frame_index}', 'frame': frame})
        lines.append('#endif /* SC64_ZP_CAMPAIGN_ACTORS */')
    if z5_selected or z5_building_selected:
        lines.append('#ifdef SC64_Z5_ACTORS')
        if ({item['name'] for item in (z5_selected or [])} == set(Z5_UNIT_IDS) and
                {item['name'] for item in (z5_building_selected or [])} == set(Z5_BUILDING_IDS)):
            lines.append('#define SC64_Z5_SPRITE_SET_COMPLETE 1')
        for item in (z5_selected or []):
            append_asset(item)
        for item in (z5_building_selected or []):
            for frame_index, frame in enumerate(item['frames']):
                append_asset({'name': f'{item["name"]}_frame_{frame_index}', 'frame': frame})
        lines.append('#endif /* SC64_Z5_ACTORS */')
    if z6_selected:
        lines.append('#ifdef SC64_Z6_ACTORS')
        if {item['name'] for item in z6_selected} == set(Z6_UNIT_IDS):
            lines.append('#define SC64_Z6_SPRITE_SET_COMPLETE 1')
        for item in z6_selected:
            append_asset(item)
        lines.append('#endif /* SC64_Z6_ACTORS */')
    if z6_building_selected:
        lines.append('#ifdef SC64_Z6_RUNTIME')
        if {item['name'] for item in z6_building_selected} == set(Z6_BUILDING_IDS):
            lines.append('#define SC64_Z6_BUILDING_SPRITE_SET_COMPLETE 1')
        for item in z6_building_selected:
            for frame_index, frame in enumerate(item['frames']):
                append_asset({'name': f'{item["name"]}_frame_{frame_index}', 'frame': frame})
        lines.append('#endif /* SC64_Z6_RUNTIME */')
    if z5_doodad_selected:
        lines.append('#ifdef SC64_Z5_DOODADS')
        if {item['name'] for item in z5_doodad_selected} == set(Z5_DOODAD_IDS):
            lines.append('#define SC64_Z5_DOODAD_SPRITE_SET_COMPLETE 1')
        for item in z5_doodad_selected:
            for frame_index, frame in enumerate(item['frames']):
                append_asset({'name': f'{item["name"]}_frame_{frame_index}', 'frame': frame})
        lines.append('#endif /* SC64_Z5_DOODADS */')
    if p6_selected or p6_doodad_selected:
        lines.append('#ifdef SC64_P6_ACTORS')
        if ({item['name'] for item in (p6_selected or [])} == set(P6_UNIT_IDS) and
                {item['name'] for item in (p6_doodad_selected or [])} == set(P6_DOODAD_IDS)):
            lines.append('#define SC64_P6_SPRITE_SET_COMPLETE 1')
        for item in (p6_selected or []):
            append_asset(item)
        for item in (p6_doodad_selected or []):
            for frame_index, frame in enumerate(item['frames']):
                append_asset({'name': f'{item["name"]}_frame_{frame_index}', 'frame': frame})
        lines.append('#endif /* SC64_P6_ACTORS */')
    if p7_selected or p7_building_selected:
        lines.append('#ifdef SC64_P7_ACTORS')
        if (set(P7_SHARED_UNIT_IDS).issubset({item['name'] for item in selected}) and
                {item['name'] for item in (p7_selected or [])} == set(P7_UNIT_IDS) and
                {item['name'] for item in (p7_building_selected or [])} == set(P7_BUILDING_IDS)):
            lines.append('#define SC64_P7_SPRITE_SET_COMPLETE 1')
        for item in (p7_selected or []):
            append_asset(item)
        for item in (p7_building_selected or []):
            for frame_index, frame in enumerate(item['frames']):
                append_asset({'name': f'{item["name"]}_frame_{frame_index}', 'frame': frame})
        lines.append('#endif /* SC64_P7_ACTORS */')
    lines += ['static inline const Sc64SpriteFrame *sc64_race_unit_sprite(unsigned short raw_type) {',
              '    switch (raw_type) {']
    seen = set()
    for name, raw_type in RACE_UNIT_IDS.items():
        if raw_type in seen:
            continue
        seen.add(raw_type)
        lines.append(f'    case {raw_type}: return &sc64_race_sprite_{name};')
    if t4_selected:
        lines.append('#ifdef SC64_T4_ENABLED')
        for item in t4_selected:
            lines.append(f'    case {T4_UNIT_IDS[item["name"]]}: return &sc64_race_sprite_{item["name"]};')
        lines.append('#endif /* SC64_T4_ENABLED */')
    if t6_selected:
        lines.append('#ifdef SC64_T6_ENABLED')
        for item in t6_selected:
            lines.append(f'    case {T6_UNIT_IDS[item["name"]]}: return &sc64_race_sprite_{item["name"]};')
        lines.append('#endif /* SC64_T6_ENABLED */')
    if late_selected:
        lines.append('#ifdef SC64_TERRAN_LATE_ENABLED')
        for item in late_selected:
            lines.append(f'    case {LATE_UNIT_IDS[item["name"]]}: return &sc64_race_sprite_{item["name"]};')
        lines.append('#endif /* SC64_TERRAN_LATE_ENABLED */')
    if zp_selected:
        lines.append('#ifdef SC64_ZP_CAMPAIGN_ACTORS')
        for item in zp_selected:
            raw_type = ZP_UNIT_IDS[item['name']]
            if raw_type == 19 and t6_selected:
                continue  # The T6 helper already emits raw 19 when both sets are compiled.
            lines.append(f'    case {raw_type}: return &sc64_race_sprite_{item["name"]};')
        lines.append('#endif /* SC64_ZP_CAMPAIGN_ACTORS */')
    if p6_selected:
        lines.append('#ifdef SC64_P6_ACTORS')
        for item in p6_selected:
            lines.append(f'    case {P6_UNIT_IDS[item["name"]]}: return &sc64_race_sprite_{item["name"]};')
        lines.append('#endif /* SC64_P6_ACTORS */')
    if p7_selected:
        lines.append('#ifdef SC64_P7_ACTORS')
        for item in p7_selected:
            lines.append(f'    case {P7_UNIT_IDS[item["name"]]}: return &sc64_race_sprite_{item["name"]};')
        lines.append('#endif /* SC64_P7_ACTORS */')
    lines += ['    default: return (const Sc64SpriteFrame *)0;', '    }', '}', '#endif']
    if t4_selected:
        # A null sprite means "use the generic fallback" in game.c, so it
        # cannot represent the source-invisible Map Revealer. Expose a distinct
        # mode for the later renderer integration without inventing art.
        lines[-1:-1] = [
            '#ifdef SC64_T4_ENABLED',
            'static inline unsigned char sc64_race_unit_sprite_mode(unsigned short raw_type) {',
            '    switch (raw_type) {',
            '    case 101: return 2; /* source-invisible map revealer */',
            '    default: return sc64_race_unit_sprite(raw_type) ? 1 : 0; /* sprite / fallback */',
            '    }',
            '}',
            '#endif /* SC64_T4_ENABLED */',
        ]
    if z5_selected:
        lines[-1:-1] = [
            '#ifdef SC64_Z5_ACTORS',
            'static inline const Sc64SpriteFrame *sc64_z5_actor_sprite(unsigned short raw_type) {',
            '    switch (raw_type) {',
        ] + [f'    case {Z5_UNIT_IDS[item["name"]]}: return &sc64_race_sprite_{item["name"]};'
             for item in z5_selected] + [
            '    default: return (const Sc64SpriteFrame *)0;',
            '    }',
            '}',
            '#endif /* SC64_Z5_ACTORS */',
        ]
    if z6_selected:
        lines[-1:-1] = [
            '#ifdef SC64_Z6_ACTORS',
            'static inline const Sc64SpriteFrame *sc64_z6_actor_sprite(unsigned short raw_type) {',
            '    switch (raw_type) {',
        ] + [f'    case {Z6_UNIT_IDS[item["name"]]}: return &sc64_race_sprite_{item["name"]};'
             for item in z6_selected] + [
            '    default: return (const Sc64SpriteFrame *)0;',
            '    }',
            '}',
            '#endif /* SC64_Z6_ACTORS */',
        ]
    if zp_selected:
        lines[-1:-1] = [
            '#ifdef SC64_ZP_CAMPAIGN_ACTORS',
            'static inline unsigned char sc64_zp_unit_sprite_mode(unsigned short raw_type) {',
            '    switch (raw_type) {',
            '    case 101: return 2; /* map revealer has no source-visible sprite */',
            '    default: return sc64_race_unit_sprite(raw_type) ? 1 : 0;',
            '    }', '}',
            '#endif /* SC64_ZP_CAMPAIGN_ACTORS */',
        ]
    if (t4_building_selected or late_building_selected or zp_building_selected or
            z5_building_selected or z5_doodad_selected or p6_doodad_selected or
            z6_building_selected or p7_building_selected):
        # Keep the frame index a raw source-GRP index. IScript-to-state
        # selection is intentionally left to the runtime trigger adapter.
        at = len(lines) - 1
        building_sprite_guard = '#if defined(SC64_T4_ENABLED) || defined(SC64_TERRAN_LATE_ENABLED) || defined(SC64_ZP_CAMPAIGN_ACTORS) || defined(SC64_Z5_ACTORS)'
        if z5_doodad_selected:
            building_sprite_guard += ' || defined(SC64_Z5_DOODADS)'
        if p6_doodad_selected:
            building_sprite_guard += ' || defined(SC64_P6_ACTORS)'
        if z6_building_selected:
            building_sprite_guard += ' || defined(SC64_Z6_RUNTIME)'
        if p7_building_selected:
            building_sprite_guard += ' || defined(SC64_P7_ACTORS)'
        helper = [building_sprite_guard,
                  'static inline const Sc64SpriteFrame *sc64_race_building_sprite(unsigned short raw_type, unsigned short frame_index) {',
                  '    switch (raw_type) {']
        if t4_building_selected:
            helper.append('#ifdef SC64_T4_ENABLED')
        for item in (t4_building_selected or []):
            if item['name'] in T4_BUILDING_IDS and T4_BUILDING_IDS[item['name']] in Z5_DOODAD_IDS.values():
                helper.append('#ifndef SC64_Z5_DOODADS')
            helper += [f'    case {T4_BUILDING_IDS[item["name"]]}:',
                       '        switch (frame_index) {']
            for frame_index in range(len(item['frames'])):
                helper.append(f'        case {frame_index}: return &sc64_race_sprite_{item["name"]}_frame_{frame_index};')
            helper += ['        default: return (const Sc64SpriteFrame *)0;', '        }']
            if item['name'] in T4_BUILDING_IDS and T4_BUILDING_IDS[item['name']] in Z5_DOODAD_IDS.values():
                helper.append('#endif /* !SC64_Z5_DOODADS */')
        if t4_building_selected:
            helper.append('#endif /* SC64_T4_ENABLED */')
        if late_building_selected:
            helper += ['#ifdef SC64_TERRAN_LATE_ENABLED']
            for item in late_building_selected:
                if p7_building_selected and LATE_BUILDING_IDS[item['name']] in P7_BUILDING_IDS.values():
                    helper.append('#ifndef SC64_P7_ACTORS')
                helper += [f'    case {LATE_BUILDING_IDS[item["name"]]}:',
                           '        switch (frame_index) {']
                for frame_index in range(len(item['frames'])):
                    helper.append(f'        case {frame_index}: return &sc64_race_sprite_{item["name"]}_frame_{frame_index};')
                helper += ['        default: return (const Sc64SpriteFrame *)0;', '        }']
                if p7_building_selected and LATE_BUILDING_IDS[item['name']] in P7_BUILDING_IDS.values():
                    helper.append('#endif /* !SC64_P7_ACTORS */')
            helper += ['#endif /* SC64_TERRAN_LATE_ENABLED */']
        if z6_building_selected:
            helper += ['#if defined(SC64_Z6_RUNTIME) && !defined(SC64_TERRAN_LATE_ENABLED)']
            for item in z6_building_selected:
                if p7_building_selected and Z6_BUILDING_IDS[item['name']] in P7_BUILDING_IDS.values():
                    helper.append('#ifndef SC64_P7_ACTORS')
                helper += [f'    case {Z6_BUILDING_IDS[item["name"]]}:',
                           '        switch (frame_index) {']
                for frame_index in range(len(item['frames'])):
                    helper.append(f'        case {frame_index}: return &sc64_race_sprite_{item["name"]}_frame_{frame_index};')
                helper += ['        default: return (const Sc64SpriteFrame *)0;', '        }']
                if p7_building_selected and Z6_BUILDING_IDS[item['name']] in P7_BUILDING_IDS.values():
                    helper.append('#endif /* !SC64_P7_ACTORS */')
            helper += ['#endif /* SC64_Z6_RUNTIME && !SC64_TERRAN_LATE_ENABLED */']
        if zp_building_selected:
            helper += ['#ifdef SC64_ZP_CAMPAIGN_ACTORS']
            for item in zp_building_selected:
                helper += [f'    case {ZP_BUILDING_IDS[item["name"]]}:', '        switch (frame_index) {']
                for frame_index in range(len(item['frames'])):
                    helper.append(f'        case {frame_index}: return &sc64_race_sprite_{item["name"]}_frame_{frame_index};')
                helper += ['        default: return (const Sc64SpriteFrame *)0;', '        }']
            helper += ['#endif /* SC64_ZP_CAMPAIGN_ACTORS */']
        if z5_building_selected:
            helper += ['#ifdef SC64_Z5_ACTORS']
            for item in z5_building_selected:
                helper += [f'    case {Z5_BUILDING_IDS[item["name"]]}:', '        switch (frame_index) {']
                for frame_index in range(len(item['frames'])):
                    helper.append(f'        case {frame_index}: return &sc64_race_sprite_{item["name"]}_frame_{frame_index};')
                helper += ['        default: return (const Sc64SpriteFrame *)0;', '        }']
            helper += ['#endif /* SC64_Z5_ACTORS */']
        if z5_doodad_selected:
            helper += ['#ifdef SC64_Z5_DOODADS']
            for item in z5_doodad_selected:
                helper += [f'    case {Z5_DOODAD_IDS[item["name"]]}:',
                           '        switch (frame_index) {']
                for frame_index in range(len(item['frames'])):
                    helper.append(f'        case {frame_index}: return &sc64_race_sprite_{item["name"]}_frame_{frame_index};')
                helper += ['        default: return (const Sc64SpriteFrame *)0;', '        }']
            helper += ['#endif /* SC64_Z5_DOODADS */']
        if p6_doodad_selected:
            helper += ['#ifdef SC64_P6_ACTORS']
            for item in p6_doodad_selected:
                helper += [f'    case {P6_DOODAD_IDS[item["name"]]}:',
                           '        switch (frame_index) {']
                for frame_index in range(len(item['frames'])):
                    helper.append(f'        case {frame_index}: return &sc64_race_sprite_{item["name"]}_frame_{frame_index};')
                helper += ['        default: return (const Sc64SpriteFrame *)0;', '        }']
            helper += ['#endif /* SC64_P6_ACTORS */']
        if p7_building_selected:
            helper += ['#ifdef SC64_P7_ACTORS']
            for item in p7_building_selected:
                helper += [f'    case {P7_BUILDING_IDS[item["name"]]}:',
                           '        switch (frame_index) {']
                for frame_index in range(len(item['frames'])):
                    helper.append(f'        case {frame_index}: return &sc64_race_sprite_{item["name"]}_frame_{frame_index};')
                helper += ['        default: return (const Sc64SpriteFrame *)0;', '        }']
            helper += ['#endif /* SC64_P7_ACTORS */']
        helper += ['    default: return (const Sc64SpriteFrame *)0;', '    }', '}', '#endif']
        lines[at:at] = helper
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text('\n'.join(lines) + '\n', encoding='ascii')


def generate(rom: Path = ROM, out: Path = OUT, header: Path = HEADER,
             remap_path: Path = PALETTE_REMAP, include_t4: bool = True,
             include_t6: bool = False, include_late: bool = False,
             include_zp: bool = False, include_z5: bool = False,
             include_z6: bool = False, include_z5_doodads: bool = False,
             include_p6: bool = False,
             include_z6_buildings: bool = False,
             include_p7: bool = False) -> dict:
    raw = rom.read_bytes()
    extractor = base.load_extractor(base.UPSTREAM)
    normalized = extractor.load_rom(str(rom))
    sha1 = hashlib.sha1(normalized).hexdigest()
    if sha1 != base.EXPECTED_USA_ROM_SHA1:
        raise ValueError(f'Expected verified USA ROM {base.EXPECTED_USA_ROM_SHA1}, got {sha1}')
    remap = remap_path.read_bytes()
    if len(remap) != 256:
        raise ValueError('Backwater sprite remap must have 256 entries')
    if include_p6:
        if not include_t4:
            raise ValueError('P6 sprites require the T4 prop sprite set')
        # P6 reuses existing base Ghost, T4 doors/traps, ZP beacons and Z5
        # indexed doodads. Selecting the P6 bundle includes those source sets
        # so its generated header cannot silently omit an inherited mapping.
        include_zp = True
        include_z5_doodads = True
    archive = extractor.BoltArchive(normalized)
    refs = base.resolve_grp_indices(archive, RACE_UNIT_IDS)
    t4_refs = base.resolve_grp_indices(archive, T4_UNIT_IDS) if include_t4 else {}
    t4_building_refs = base.resolve_grp_indices(archive, T4_BUILDING_IDS) if include_t4 else {}
    t6_refs = base.resolve_grp_indices(archive, T6_UNIT_IDS) if include_t6 else {}
    if include_late and (not include_t4 or not include_t6):
        raise ValueError('late Terran sprites require the combined T4/T6 sprite sets')
    late_refs = base.resolve_grp_indices(archive, LATE_UNIT_IDS) if include_late else {}
    late_building_refs = base.resolve_grp_indices(archive, LATE_BUILDING_IDS) if include_late else {}
    zp_refs = base.resolve_grp_indices(archive, ZP_UNIT_IDS) if include_zp else {}
    zp_building_refs = base.resolve_grp_indices(archive, ZP_BUILDING_IDS) if include_zp else {}
    z5_refs = base.resolve_grp_indices(archive, Z5_UNIT_IDS) if include_z5 else {}
    z5_building_refs = base.resolve_grp_indices(archive, Z5_BUILDING_IDS) if include_z5 else {}
    if include_z5_doodads and not include_z5 and not include_p6:
        raise ValueError('Z5 doodad sprites require the Z5 actor sprite set')
    z5_doodad_refs = base.resolve_grp_indices(archive, Z5_DOODAD_IDS) if include_z5_doodads else {}
    z6_refs = base.resolve_grp_indices(archive, Z6_UNIT_IDS) if include_z6 else {}
    z6_building_refs = (base.resolve_grp_indices(archive, Z6_BUILDING_IDS)
                        if include_z6_buildings else {})
    p6_refs = base.resolve_grp_indices(archive, P6_UNIT_IDS) if include_p6 else {}
    p6_doodad_refs = base.resolve_grp_indices(archive, P6_DOODAD_IDS) if include_p6 else {}
    p7_refs = base.resolve_grp_indices(archive, P7_UNIT_IDS) if include_p7 else {}
    p7_building_refs = base.resolve_grp_indices(archive, P7_BUILDING_IDS) if include_p7 else {}
    palette = base.decode_rgba5551_palette(base._entry_payload(archive, '00B/004'))
    entries = {entry.path: entry for entry in archive.entries()}
    selected, t4_selected, t4_building_selected, t6_selected = [], [], [], []
    late_selected, late_building_selected, manifest_assets = [], [], []
    zp_selected, zp_building_selected = [], []
    z5_selected, z5_building_selected = [], []
    z5_doodad_selected = []
    z6_selected = []
    z6_building_selected = []
    p6_selected, p6_doodad_selected = [], []
    p7_selected, p7_building_selected = [], []
    out.mkdir(parents=True, exist_ok=True)
    def load_asset(name, ref, target):
        grp_data = archive.read(entries[ref['bolt_path']])
        grp = base.parse_grp(grp_data)
        frame = grp['frames'][0]
        png = out / f'{name}.png'
        base.encode_png(png, grp['width'], grp['height'], base.render_frame(frame, palette))
        target.append({'name': name, 'frame': frame})
        manifest_assets.append({'name': name, **ref, 'sha1': hashlib.sha1(grp_data).hexdigest(),
                                'frame_count': len(grp['frames']),
                                'canvas': [grp['width'], grp['height']],
                                'selected_frame': 0, 'frame_rect': [frame['x'], frame['y'],
                                frame['frame_width'], frame['frame_height']],
                                'opaque_pixels': sum(frame['alpha']), 'preview': png.name})
    for name, ref in refs.items():
        load_asset(name, ref, selected)
    for name, ref in t4_refs.items():
        load_asset(name, ref, t4_selected)
    for name, ref in t4_building_refs.items():
        grp_data = archive.read(entries[ref['bolt_path']])
        grp = base.parse_grp(grp_data)
        frame_list = grp['frames']
        if not frame_list:
            raise ValueError(f'T4 prop raw {ref["unit_id"]} has an empty GRP')
        preview = out / f'{name}.png'
        base.encode_png(preview, grp['width'], grp['height'], base.render_frame(frame_list[0], palette))
        t4_building_selected.append({'name': name, 'frames': frame_list})
        manifest_assets.append({'name': name, **ref, 'sha1': hashlib.sha1(grp_data).hexdigest(),
                                'frame_count': len(frame_list), 'canvas': [grp['width'], grp['height']],
                                'selected_frame': 'all-source-frames',
                                'frame_rect': [frame_list[0]['x'], frame_list[0]['y'],
                                               frame_list[0]['frame_width'], frame_list[0]['frame_height']],
                                'opaque_pixels': sum(frame_list[0]['alpha']),
                                'preview': preview.name})
    legacy_asset_count = len(manifest_assets)
    for name, ref in t6_refs.items():
        load_asset(name, ref, t6_selected)
    if include_t6 and {int(ref['unit_id']) for ref in t6_refs.values()} != set(T6_UNIT_IDS.values()):
        raise ValueError('T6 source GRPs did not resolve')
    for name, ref in late_refs.items():
        load_asset(name, ref, late_selected)
    for name, ref in late_building_refs.items():
        grp_data = archive.read(entries[ref['bolt_path']])
        grp = base.parse_grp(grp_data)
        frame_list = grp['frames']
        if not frame_list:
            raise ValueError(f'late Terran prop raw {ref["unit_id"]} has an empty GRP')
        preview = out / f'{name}.png'
        base.encode_png(preview, grp['width'], grp['height'], base.render_frame(frame_list[0], palette))
        late_building_selected.append({'name': name, 'frames': frame_list})
        manifest_assets.append({'name': name, **ref, 'sha1': hashlib.sha1(grp_data).hexdigest(),
                                'frame_count': len(frame_list), 'canvas': [grp['width'], grp['height']],
                                'selected_frame': 'all-source-frames',
                                'frame_rect': [frame_list[0]['x'], frame_list[0]['y'],
                                               frame_list[0]['frame_width'], frame_list[0]['frame_height']],
                                'opaque_pixels': sum(frame_list[0]['alpha']),
                                'preview': preview.name})
    if include_late and (set(late_refs) != set(LATE_UNIT_IDS) or
                         set(late_building_refs) != set(LATE_BUILDING_IDS)):
        raise ValueError('late Terran source GRPs did not resolve completely')
    for name, ref in zp_refs.items():
        load_asset(name, ref, zp_selected)
    for name, ref in zp_building_refs.items():
        grp_data = archive.read(entries[ref['bolt_path']])
        grp = base.parse_grp(grp_data)
        if not grp['frames']:
            raise ValueError(f'Z/P scenario building raw {ref["unit_id"]} has an empty GRP')
        frames = grp['frames']
        preview = out / f'{name}.png'
        base.encode_png(preview, grp['width'], grp['height'], base.render_frame(frames[0], palette))
        zp_building_selected.append({'name': name, 'frames': frames})
        manifest_assets.append({'name': name, **ref, 'sha1': hashlib.sha1(grp_data).hexdigest(),
                                'frame_count': len(frames), 'canvas': [grp['width'], grp['height']],
                                'selected_frame': 'all-source-frames',
                                'frame_rect': [frames[0]['x'], frames[0]['y'], frames[0]['frame_width'], frames[0]['frame_height']],
                                'opaque_pixels': sum(frames[0]['alpha']), 'preview': preview.name})
    if include_zp and (set(zp_refs) != set(ZP_UNIT_IDS) or set(zp_building_refs) != set(ZP_BUILDING_IDS)):
        raise ValueError('Z/P campaign actor source GRPs did not resolve completely')
    for name, ref in z5_refs.items():
        load_asset(name, ref, z5_selected)
    for name, ref in z5_building_refs.items():
        grp_data = archive.read(entries[ref['bolt_path']])
        grp = base.parse_grp(grp_data)
        if not grp['frames']:
            raise ValueError(f'Z5 source building raw {ref["unit_id"]} has an empty GRP')
        frames = grp['frames']
        preview = out / f'{name}.png'
        base.encode_png(preview, grp['width'], grp['height'], base.render_frame(frames[0], palette))
        z5_building_selected.append({'name': name, 'frames': frames})
        manifest_assets.append({'name': name, **ref, 'sha1': hashlib.sha1(grp_data).hexdigest(),
                                'frame_count': len(frames), 'canvas': [grp['width'], grp['height']],
                                'selected_frame': 'all-source-frames',
                                'frame_rect': [frames[0]['x'], frames[0]['y'],
                                               frames[0]['frame_width'], frames[0]['frame_height']],
                                'opaque_pixels': sum(frames[0]['alpha']), 'preview': preview.name})
    if include_z5 and (set(z5_refs) != set(Z5_UNIT_IDS) or
                       set(z5_building_refs) != set(Z5_BUILDING_IDS)):
        raise ValueError('Z5 actor source GRPs did not resolve completely')
    for name, ref in z5_doodad_refs.items():
        grp_data = archive.read(entries[ref['bolt_path']])
        grp = base.parse_grp(grp_data)
        if not grp['frames']:
            raise ValueError(f'Z5 source doodad raw {ref["unit_id"]} has an empty GRP')
        frames = grp['frames']
        preview = out / f'{name}.png'
        base.encode_png(preview, grp['width'], grp['height'], base.render_frame(frames[0], palette))
        z5_doodad_selected.append({'name': name, 'frames': frames})
        manifest_assets.append({'name': name, **ref, 'sha1': hashlib.sha1(grp_data).hexdigest(),
                                'frame_count': len(frames), 'canvas': [grp['width'], grp['height']],
                                'selected_frame': 'all-source-frames',
                                'frame_rect': [frames[0]['x'], frames[0]['y'], frames[0]['frame_width'], frames[0]['frame_height']],
                                'opaque_pixels': sum(frames[0]['alpha']), 'preview': preview.name})
    if include_z5_doodads and set(z5_doodad_refs) != set(Z5_DOODAD_IDS):
        raise ValueError('Z5 source doodad GRPs did not resolve completely')
    for name, ref in z6_refs.items():
        load_asset(name, ref, z6_selected)
    if include_z6 and set(z6_refs) != set(Z6_UNIT_IDS):
        raise ValueError('Z6 actor source GRPs did not resolve completely')
    for name, ref in z6_building_refs.items():
        grp_data = archive.read(entries[ref['bolt_path']])
        grp = base.parse_grp(grp_data)
        if not grp['frames']:
            raise ValueError(f'Z6 source building raw {ref["unit_id"]} has an empty GRP')
        frames = grp['frames']
        preview = out / f'{name}.png'
        base.encode_png(preview, grp['width'], grp['height'], base.render_frame(frames[0], palette))
        z6_building_selected.append({'name': name, 'frames': frames})
        manifest_assets.append({'name': name, **ref, 'sha1': hashlib.sha1(grp_data).hexdigest(),
                                'frame_count': len(frames), 'canvas': [grp['width'], grp['height']],
                                'selected_frame': 'all-source-frames',
                                'frame_rect': [frames[0]['x'], frames[0]['y'],
                                               frames[0]['frame_width'], frames[0]['frame_height']],
                                'opaque_pixels': sum(frames[0]['alpha']), 'preview': preview.name})
    if include_z6_buildings and set(z6_building_refs) != set(Z6_BUILDING_IDS):
        raise ValueError('Z6 source building GRPs did not resolve completely')
    for name, ref in p6_refs.items():
        load_asset(name, ref, p6_selected)
    if include_p6 and set(p6_refs) != set(P6_UNIT_IDS):
        raise ValueError('P6 actor source GRPs did not resolve completely')
    for name, ref in p6_doodad_refs.items():
        grp_data = archive.read(entries[ref['bolt_path']])
        grp = base.parse_grp(grp_data)
        if not grp['frames']:
            raise ValueError(f'P6 source doodad raw {ref["unit_id"]} has an empty GRP')
        frames = grp['frames']
        preview = out / f'{name}.png'
        base.encode_png(preview, grp['width'], grp['height'], base.render_frame(frames[0], palette))
        p6_doodad_selected.append({'name': name, 'frames': frames})
        manifest_assets.append({'name': name, **ref, 'sha1': hashlib.sha1(grp_data).hexdigest(),
                                'frame_count': len(frames), 'canvas': [grp['width'], grp['height']],
                                'selected_frame': 'all-source-frames',
                                'frame_rect': [frames[0]['x'], frames[0]['y'],
                                               frames[0]['frame_width'], frames[0]['frame_height']],
                                'opaque_pixels': sum(frames[0]['alpha']), 'preview': preview.name})
    if include_p6 and set(p6_doodad_refs) != set(P6_DOODAD_IDS):
        raise ValueError('P6 source doodad GRPs did not resolve completely')
    for name, ref in p7_refs.items():
        load_asset(name, ref, p7_selected)
    if include_p7 and set(p7_refs) != set(P7_UNIT_IDS):
        raise ValueError('P7 actor source GRPs did not resolve completely')
    if include_p7 and {int(ref['unit_id']) for ref in p7_refs.values()} != set(P7_UNIT_IDS.values()):
        raise ValueError('P7 actor source GRPs resolved to unexpected raw ids')
    for name, ref in p7_building_refs.items():
        grp_data = archive.read(entries[ref['bolt_path']])
        grp = base.parse_grp(grp_data)
        if not grp['frames']:
            raise ValueError(f'P7 source building raw {ref["unit_id"]} has an empty GRP')
        frames = grp['frames']
        preview = out / f'{name}.png'
        base.encode_png(preview, grp['width'], grp['height'], base.render_frame(frames[0], palette))
        p7_building_selected.append({'name': name, 'frames': frames})
        manifest_assets.append({'name': name, **ref, 'sha1': hashlib.sha1(grp_data).hexdigest(),
                                'frame_count': len(frames), 'canvas': [grp['width'], grp['height']],
                                'selected_frame': 'all-source-frames',
                                'frame_rect': [frames[0]['x'], frames[0]['y'],
                                               frames[0]['frame_width'], frames[0]['frame_height']],
                                'opaque_pixels': sum(frames[0]['alpha']), 'preview': preview.name})
    if include_p7 and set(p7_building_refs) != set(P7_BUILDING_IDS):
        raise ValueError('P7 source building GRPs did not resolve completely')
    if include_p7 and {int(ref['unit_id']) for ref in p7_building_refs.values()} != set(P7_BUILDING_IDS.values()):
        raise ValueError('P7 source building GRPs resolved to unexpected raw ids')
    write_header(header, selected, remap, t4_selected, t4_building_selected,
                 t6_selected, late_selected, late_building_selected,
                 zp_selected, zp_building_selected, z5_selected, z5_building_selected,
                 z6_selected, z5_doodad_selected, p6_selected, p6_doodad_selected,
                 z6_building_selected, p7_selected, p7_building_selected)
    mapped_raw_types = set(RACE_UNIT_IDS.values()) | set(T4_UNIT_IDS.values()) | set(T4_BUILDING_IDS.values())
    actual_sprite_raw_types = ({int(ref['unit_id']) for ref in refs.values()} |
                               {int(ref['unit_id']) for ref in t4_refs.values()})
    expected_t4_visible = set(T4_EXPECTED_VISIBLE_RAW_TYPES) if include_t4 else set()
    missing_t4_mapping = sorted(expected_t4_visible - mapped_raw_types)
    missing_t4_sprite = sorted(expected_t4_visible - actual_sprite_raw_types)
    missing_t4_visible = sorted(set(missing_t4_mapping) | set(missing_t4_sprite))
    if missing_t4_visible:
        raise ValueError(f'T4 visible raw types lack sprite mappings: {missing_t4_visible}')
    missing_t4_props = sorted(set(T4_BUILDING_IDS.values()) -
                              {int(ref['unit_id']) for ref in t4_building_refs.values()}) if include_t4 else []
    if missing_t4_props:
        raise ValueError(f'T4 prop raw types lack source GRPs: {missing_t4_props}')
    report = {'source': {'rom_sha1': sha1, 'rom_input_sha1': hashlib.sha1(raw).hexdigest(),
                         'upstream_commit': base.UPSTREAM_COMMIT},
              'palette': 'Shared Backwater GBA sprite palette; map terrain colors are quantized offline to avoid runtime sprite recoloring.',
              'raw_unit_ids': RACE_UNIT_IDS,
              't4_raw_unit_ids': T4_UNIT_IDS if include_t4 else {},
              't4_raw_building_ids': T4_BUILDING_IDS if include_t4 else {},
              't4_invisible_raw_types': list(T4_INVISIBLE_RAW_TYPES) if include_t4 else [],
              't4_expected_visible_raw_types': list(T4_EXPECTED_VISIBLE_RAW_TYPES) if include_t4 else [],
              'resolved_t4_visible_raw_types': sorted(set(T4_EXPECTED_VISIBLE_RAW_TYPES) & actual_sprite_raw_types) if include_t4 else [],
              'base_assets_manifest_sha256': asset_manifest_sha256(manifest_assets[:len(selected)]),
              't4_assets_manifest_sha256': asset_manifest_sha256(manifest_assets[len(selected):legacy_asset_count]) if include_t4 else asset_manifest_sha256([]),
              'assets': manifest_assets,
              'generated_header': str(header)}
    if include_t6:
        report['t6_raw_unit_ids'] = T6_UNIT_IDS
        t6_asset_end = legacy_asset_count + len(t6_selected)
        report['t6_assets_manifest_sha256'] = asset_manifest_sha256(
            manifest_assets[legacy_asset_count:t6_asset_end])
    if include_late:
        report['late_terran_raw_unit_ids'] = LATE_UNIT_IDS
        report['late_terran_raw_building_ids'] = LATE_BUILDING_IDS
        report['late_terran_assets_manifest_sha256'] = named_asset_manifest_sha256(
            manifest_assets, set(LATE_UNIT_IDS) | set(LATE_BUILDING_IDS))
    if include_zp:
        report['zp_campaign_raw_unit_ids'] = ZP_UNIT_IDS
        report['zp_campaign_raw_building_ids'] = ZP_BUILDING_IDS
        report['zp_campaign_invisible_raw_types'] = list(ZP_INVISIBLE_RAW_TYPES)
        report['zp_campaign_assets_manifest_sha256'] = named_asset_manifest_sha256(
            manifest_assets, set(ZP_UNIT_IDS) | set(ZP_BUILDING_IDS))
    if include_z5:
        report['z5_raw_unit_ids'] = Z5_UNIT_IDS
        report['z5_raw_building_ids'] = Z5_BUILDING_IDS
        report['z5_assets_manifest_sha256'] = named_asset_manifest_sha256(
            manifest_assets, set(Z5_UNIT_IDS) | set(Z5_BUILDING_IDS))
    if include_z5_doodads:
        report['z5_doodad_raw_building_ids'] = Z5_DOODAD_IDS
        report['z5_doodad_assets_manifest_sha256'] = named_asset_manifest_sha256(
            manifest_assets, set(Z5_DOODAD_IDS))
    if include_z6:
        report['z6_raw_unit_ids'] = Z6_UNIT_IDS
        report['z6_assets_manifest_sha256'] = named_asset_manifest_sha256(
            manifest_assets, set(Z6_UNIT_IDS))
    if include_z6_buildings:
        report['z6_raw_building_ids'] = Z6_BUILDING_IDS
        report['z6_building_assets_manifest_sha256'] = named_asset_manifest_sha256(
            manifest_assets, set(Z6_BUILDING_IDS))
    if include_p6:
        report['p6_raw_unit_ids'] = P6_UNIT_IDS
        report['p6_raw_doodad_ids'] = P6_DOODAD_IDS
        report['p6_assets_manifest_sha256'] = named_asset_manifest_sha256(
            manifest_assets, set(P6_UNIT_IDS) | set(P6_DOODAD_IDS))
    if include_p7:
        report['p7_raw_unit_ids'] = {**P7_SHARED_UNIT_IDS, **P7_UNIT_IDS}
        report['p7_raw_building_ids'] = P7_BUILDING_IDS
        report['p7_assets_manifest_sha256'] = named_asset_manifest_sha256(
            manifest_assets, set(P7_SHARED_UNIT_IDS) | set(P7_UNIT_IDS) |
            set(P7_BUILDING_IDS))
    (out / 'manifest.json').write_text(json.dumps(report, indent=2) + '\n')
    return report


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rom', type=Path, default=ROM)
    parser.add_argument('--out', type=Path, default=OUT)
    parser.add_argument('--header', type=Path, default=HEADER)
    parser.add_argument('--remap', type=Path, default=PALETTE_REMAP)
    parser.add_argument('--base-only', action='store_true',
                        help='omit the SC64_T4_ENABLED-gated T4 sprite payloads')
    parser.add_argument('--include-t6', action='store_true',
                        help='include SC64_T6_ENABLED-gated Norad, Raynor and Control Tower source sprites')
    parser.add_argument('--include-late-terran', action='store_true',
                        help='include SC64_TERRAN_LATE_ENABLED-gated late Terran campaign source sprites')
    parser.add_argument('--include-zp-campaign-actors', action='store_true',
                        help='include SC64_ZP_CAMPAIGN_ACTORS-gated Zerg/Protoss campaign actor sprites')
    parser.add_argument('--include-z5-actors', action='store_true',
                        help='include the separately gated SC64_Z5_ACTORS sprite set')
    parser.add_argument('--include-z5-doodads', action='store_true',
                        help='include all indexed prop GRPs behind SC64_Z5_DOODADS (requires --include-z5-actors)')
    parser.add_argument('--include-z6-actors', action='store_true',
                        help='include the separately gated SC64_Z6_ACTORS Guardian sprite set')
    parser.add_argument('--include-z6-buildings', action='store_true',
                        help='include SC64_Z6_RUNTIME-gated Robotics Support Bay and Shield Battery frame banks')
    parser.add_argument('--include-p6-actors', action='store_true',
                        help='include SC64_P6_ACTORS sprites and reuse the T4/ZP/Z5 actor and prop sets')
    parser.add_argument('--include-p7-actors', action='store_true',
                        help='include SC64_P7_ACTORS Arbiter and Protoss structure source sprites')
    args = parser.parse_args()
    try:
        report = generate(args.rom, args.out, args.header, args.remap,
                          include_t4=not args.base_only, include_t6=args.include_t6,
                          include_late=args.include_late_terran,
                          include_zp=args.include_zp_campaign_actors,
                          include_z5=args.include_z5_actors,
                          include_z6=args.include_z6_actors,
                          include_z5_doodads=args.include_z5_doodads,
                          include_p6=args.include_p6_actors,
                          include_z6_buildings=args.include_z6_buildings,
                          include_p7=args.include_p7_actors)
    except (OSError, ValueError, RuntimeError, KeyError) as exc:
        print(f'sc64_race_sprites: {exc}', file=sys.stderr)
        return 1
    print(f"Generated {len(report['assets'])} race sprites into {args.header}")
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
