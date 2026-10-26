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

IMAGE_COUNT = 999

ROM = ROOT / '.local/sc64/rom/StarCraft64-USA.z64'
PALETTE_REMAP = ROOT / '.local/sc64/backwater/generated/palette-remap.bin'
OUT = ROOT / '.local/sc64/races/generated/sprites'
HEADER = ROOT / '.local/sc64/races/generated/sc64_race_sprites_generated.h'

# Brood War raw unit ids. Entries cover actual map records and the normal
# skirmish build menu. Names are stable C identifiers used by the game.
RACE_UNIT_IDS = {
    'marine': 0, 'ghost': 1, 'vulture': 2, 'goliath': 3, 'siege_tank': 5,
    'siege_tank_mode': 30, 'medic': 34, 'spider_mine': 13, 'kerrigan': 16,
    'scv': 7, 'wraith': 8, 'science_vessel': 9, 'dropship': 11,
    # Canonical enum aliases can resolve to either raw hero identity even
    # when the corresponding optional campaign adapter is disabled.
    'raynor_vulture_t6': 19, 'raynor_raw20': 20,
    'firebat': 32,
    'battlecruiser': 12, 'hero_fenix': 77, 'fenix': 77,
    'tassadar': 78, 'tassadar_hero_zp': 79,
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
    'command_center': 106, 'supply_depot': 109, 'refinery': 110,
    'factory': 113, 'barracks': 111,
    'academy': 112, 'starport': 114, 'missile_turret': 124, 'bunker': 125,
    'infested_command_center': 130, 'kakaru': 53, 'young_chrysalis': 216,
    'mineral_field_1': 176, 'mineral_field_2': 177, 'mineral_field_3': 178,
    'geyser': 188, 'special_zerg_beacon': 194,
}

# Additional source-backed IDs needed by The Jacobs Installation (T4). Keep
# T4-only scenario actors out of the shared race sprite list. Raw 101 is
# deliberately not an image:
# it is a map-revealer actor and must remain invisible in the source behavior.
T4_UNIT_IDS = {
    'civilian': 15,
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
# The crashed objective is not raw-29 Hero_Norad_II. Raw-19 Raynor now lives in
# the unconditional bank because base race enum aliases can resolve to it.
# Queen's Nest (138) already has a shared source sprite above.
T6_UNIT_IDS = {'crashed_norad_ii': 126, 'control_tower_t6': 115}

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
ZP_UNIT_IDS = {'infested_kerrigan': 51, 'raynor_vulture_zp': 19}
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


def prepack_unit_sprite_words(width: int, height: int,
                              pixels: bytes) -> tuple[tuple[int, ...], ...]:
    """Pack indexed unit pixels into four x-alignment phases.

    Each phase is row-major pairs of u32 color and alpha-byte mask words.
    Color bytes in transparent lanes are zero, matching the indexed-pixel
    transparency rule and allowing a mask-free merge.
    """
    if not 1 <= width <= 256 or not 1 <= height <= 256 or len(pixels) != width * height:
        raise ValueError('unit sprite dimensions/pixels are outside the packed format bounds')
    phases = []
    for phase in range(4):
        groups = (phase + width + 3) // 4
        words = []
        for y in range(height):
            for group in range(groups):
                color = mask = 0
                for lane in range(4):
                    sx = group * 4 + lane - phase
                    pixel = pixels[y * width + sx] if 0 <= sx < width else 0
                    color |= pixel << (lane * 8)
                    if pixel:
                        mask |= 0xff << (lane * 8)
                words.extend((color, mask))
        phases.append(tuple(words))
    return tuple(phases)


def image_init_frame(image_id: int, image_iscript: tuple[int, ...],
                     iscript_payload: bytes, scripts: dict[int, int]) -> int:
    """Return the first primary GRP frame authored by an image's Init path."""
    from tools.sc64_construction_sprites import (
        animation_instructions, image_animation_starts,
    )
    if not 0 <= image_id < len(image_iscript):
        raise ValueError(f'image id {image_id} outside IScript table')
    script_id = image_iscript[image_id]
    if script_id not in scripts:
        return 0
    starts = image_animation_starts(iscript_payload, scripts[script_id])
    start = starts.get('Init')
    if start is None:
        return 0
    for opcode, args in animation_instructions(iscript_payload, start):
        if opcode == 0:  # playfram
            return args[0]
    return 0


def animation_frame_timeline(payload: bytes, start: int, idle_start: int,
                             initial_frame: int = 0) -> list[dict[str, int]]:
    """Read primary playfram/wait timing until an animation jumps to idle."""
    from tools.sc64_construction_sprites import decode_instruction
    pc = start
    current_frame = initial_frame
    duration = 0
    steps = []
    visited = set()
    for _ in range(256):
        if pc in visited:
            break
        visited.add(pc)
        opcode, args, next_pc, jump = decode_instruction(payload, pc)
        if opcode == 0:  # playfram
            if duration:
                steps.append({'frame': current_frame, 'duration': duration})
            current_frame = args[0]
            duration = 0
        elif opcode == 5:  # wait u8; N means N+1 source updates
            duration += args[0] + 1
        elif opcode in (22, 54):
            break
        elif opcode == 7 and jump is not None:
            if jump == idle_start:
                break
            pc = jump
            continue
        pc = next_pc
    steps.append({'frame': current_frame, 'duration': duration})
    return steps


def image_animation_timeline(image_id: int, animation: str,
                             image_iscript: tuple[int, ...],
                             iscript_payload: bytes,
                             scripts: dict[int, int]) -> list[dict[str, int]]:
    """Resolve one image animation's timed frames until its IsWorking path."""
    from tools.sc64_construction_sprites import image_animation_starts
    script_id = image_iscript[image_id]
    if script_id not in scripts:
        return []
    starts = image_animation_starts(iscript_payload, scripts[script_id])
    start, idle_start = starts.get(animation), starts.get('IsWorking')
    if start is None or idle_start is None:
        return []
    return animation_frame_timeline(
        iscript_payload, start, idle_start,
        image_init_frame(image_id, image_iscript, iscript_payload, scripts))


def compose_init_layers(frame: dict, layers: list[tuple[dict, int, int, bool]]) -> dict:
    """Flatten source Init imgul/imgol layers while preserving their anchor.

    Each tuple is (layer_frame, x, y, is_underlay). Layer images are centered
    on the same source anchor as the primary GRP, then shifted by IScript's
    authored offsets. The returned frame keeps a canvas centered on that same
    anchor and crops only the nontransparent bounds for the C payload.
    """
    width, height = frame['width'], frame['height']
    positions = []
    pad_x = pad_y = 0
    for layer, dx, dy, _underlay in layers:
        lx = (width - layer['width']) // 2 + dx
        ly = (height - layer['height']) // 2 + dy
        positions.append((layer, lx, ly, _underlay))
        pad_x = max(pad_x, -lx, lx + layer['width'] - width, 0)
        pad_y = max(pad_y, -ly, ly + layer['height'] - height, 0)
    out_w, out_h = width + 2 * pad_x, height + 2 * pad_y
    lower_pixels = bytearray(out_w * out_h)
    lower_alpha = bytearray(out_w * out_h)
    top_pixels = bytearray(out_w * out_h)
    top_alpha = bytearray(out_w * out_h)

    def paste(dst_pixels, dst_alpha, source, x0, y0):
        for sy in range(source['height']):
            for sx in range(source['width']):
                si = sy * source['width'] + sx
                if not source['alpha'][si]:
                    continue
                di = (y0 + sy) * out_w + x0 + sx
                dst_pixels[di] = source['pixels'][si]
                dst_alpha[di] = 1

    for layer, lx, ly, underlay in positions:
        if underlay:
            paste(lower_pixels, lower_alpha, layer, lx + pad_x, ly + pad_y)
        else:
            paste(top_pixels, top_alpha, layer, lx + pad_x, ly + pad_y)
    paste(lower_pixels, lower_alpha, frame, pad_x, pad_y)
    for i, opaque in enumerate(top_alpha):
        if opaque:
            lower_pixels[i] = top_pixels[i]
            lower_alpha[i] = 1
    xs, ys = [], []
    for index, opaque in enumerate(lower_alpha):
        if opaque:
            xs.append(index % out_w)
            ys.append(index // out_w)
    if not xs:
        raise ValueError('composed source Init frame is fully transparent')
    left, top, right, bottom = min(xs), min(ys), max(xs) + 1, max(ys) + 1
    return {'pixels': bytes(lower_pixels), 'alpha': bytes(lower_alpha),
            'width': out_w, 'height': out_h, 'x': left, 'y': top,
            'frame_width': right - left, 'frame_height': bottom - top}

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
                 p7_building_selected: list[dict] | None = None,
                 race_building_selected: list[dict] | None = None,
                 prepack_unit_words: bool = False) -> None:
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
    for item in (race_building_selected or []):
        for frame_index, frame in enumerate(item['frames']):
            append_asset({'name': f'{item["name"]}_frame_{frame_index}', 'frame': frame})
        for animation_name, steps in item.get('animations', {}).items():
            symbol = f'sc64_race_{item["name"]}_{animation_name}_frame'
            lines.append(f'static inline unsigned char {symbol}(unsigned int source_updates) {{')
            elapsed = 0
            for step in steps:
                if step['duration'] > 0:
                    elapsed += step['duration']
                    lines.append(f'    if (source_updates < {elapsed}u) return {step["frame"]}u;')
            lines.extend([f'    return {steps[-1]["frame"] if steps else 0}u;', '}'])
            if item['name'] == 'photon_cannon' and animation_name == 'gnd_attack_init':
                lines.extend([
                    f'#define SC64_PHOTON_SOURCE_FRAMES {elapsed + 1}u',
                    'static inline unsigned char sc64_photon_cannon_attack_frame(unsigned int elapsed_source_updates) {',
                    f'    return {symbol}(elapsed_source_updates);',
                    '}',
                ])
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
            raw_type = T4_UNIT_IDS[item['name']]
            if raw_type in seen:
                continue
            seen.add(raw_type)
            lines.append(f'    case {raw_type}: return &sc64_race_sprite_{item["name"]};')
        lines.append('#endif /* SC64_T4_ENABLED */')
    if t6_selected:
        lines.append('#ifdef SC64_T6_ENABLED')
        for item in t6_selected:
            raw_type = T6_UNIT_IDS[item['name']]
            if raw_type in seen:
                continue
            seen.add(raw_type)
            lines.append(f'    case {raw_type}: return &sc64_race_sprite_{item["name"]};')
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
            if raw_type in seen:
                continue  # Shared raw art wins across optional source-profile aliases.
            seen.add(raw_type)
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
    if (race_building_selected or t4_building_selected or late_building_selected or zp_building_selected or
            z5_building_selected or z5_doodad_selected or p6_doodad_selected or
            z6_building_selected or p7_building_selected):
        # Keep the frame index a raw source-GRP index. IScript-to-state
        # selection is intentionally left to the runtime trigger adapter.
        at = len(lines) - 1
        building_sprite_guard = '#if defined(SC64_RACES) || defined(SC64_T4_ENABLED) || defined(SC64_TERRAN_LATE_ENABLED) || defined(SC64_ZP_CAMPAIGN_ACTORS) || defined(SC64_Z5_ACTORS)'
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
        for item in (race_building_selected or []):
            helper += [f'    case {RACE_UNIT_IDS[item["name"]]}:',
                       '        switch (frame_index) {']
            for frame_index in range(len(item['frames'])):
                helper.append(f'        case {frame_index}: return &sc64_race_sprite_{item["name"]}_frame_{frame_index};')
            helper += ['        default: return (const Sc64SpriteFrame *)0;', '        }']
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
                if LATE_BUILDING_IDS[item['name']] in ZP_BUILDING_IDS.values():
                    helper.append('#ifndef SC64_ZP_CAMPAIGN_ACTORS')
                if p7_building_selected and LATE_BUILDING_IDS[item['name']] in P7_BUILDING_IDS.values():
                    helper.append('#ifndef SC64_P7_ACTORS')
                helper += [f'    case {LATE_BUILDING_IDS[item["name"]]}:',
                           '        switch (frame_index) {']
                for frame_index in range(len(item['frames'])):
                    helper.append(f'        case {frame_index}: return &sc64_race_sprite_{item["name"]}_frame_{frame_index};')
                helper += ['        default: return (const Sc64SpriteFrame *)0;', '        }']
                if p7_building_selected and LATE_BUILDING_IDS[item['name']] in P7_BUILDING_IDS.values():
                    helper.append('#endif /* !SC64_P7_ACTORS */')
                if LATE_BUILDING_IDS[item['name']] in ZP_BUILDING_IDS.values():
                    helper.append('#endif /* !SC64_ZP_CAMPAIGN_ACTORS */')
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
    if prepack_unit_words:
        selected_groups = [
            (None, RACE_UNIT_IDS, selected),
            ('SC64_T4_ENABLED', T4_UNIT_IDS, t4_selected or []),
            ('SC64_T6_ENABLED', T6_UNIT_IDS, t6_selected or []),
            ('SC64_TERRAN_LATE_ENABLED', LATE_UNIT_IDS, late_selected or []),
            ('SC64_ZP_CAMPAIGN_ACTORS', ZP_UNIT_IDS, zp_selected or []),
            ('SC64_P6_ACTORS', P6_UNIT_IDS, p6_selected or []),
            ('SC64_P7_ACTORS', P7_UNIT_IDS, p7_selected or []),
        ]
        emitted_raws = set()
        packed_groups = []
        metadata = ['#ifdef SC64_UNIT_WORD_SPRITES']
        for guard, raw_names, group in selected_groups:
            group_start = len(metadata)
            by_name = {item['name']: item for item in group}
            packed_frames = []
            for name, raw_type in raw_names.items():
                if raw_type >= 106 or raw_type in emitted_raws or name not in by_name:
                    continue
                emitted_raws.add(raw_type)
                frame = by_name[name]['frame']
                width, height = frame['frame_width'], frame['frame_height']
                source = bytearray(base.crop_frame(frame))
                for row in range(height):
                    for col in range(width):
                        si = (frame['y'] + row) * frame['width'] + frame['x'] + col
                        di = row * width + col
                        source[di] = remap[source[di]] if frame['alpha'][si] else 0
                phases = prepack_unit_sprite_words(width, height, bytes(source))
                phase_symbols = []
                for phase, words in enumerate(phases):
                    symbol = f'sc64_race_unit_words_{raw_type}_p{phase}'
                    phase_symbols.append(symbol)
                    metadata.append(f'static const u32 {symbol}[{len(words)}] = {{')
                    for i in range(0, len(words), 8):
                        metadata.append('    ' + ','.join(f'0x{word:08x}u' for word in words[i:i + 8]) + ',')
                    metadata.append('};')
                packed_frames.append((raw_type, width, height, phase_symbols))
            if packed_frames:
                if guard:
                    metadata.insert(group_start, f'#ifdef {guard}')
                for raw_type, width, height, phase_symbols in packed_frames:
                    metadata.append(
                        f'static const GbaSpriteWordFrame sc64_race_unit_words_{raw_type} = '
                        f'{{{width},{height},{{{",".join(phase_symbols)}}}}};')
                if guard:
                    metadata.append(f'#endif /* {guard} */')
                packed_groups.append((guard, packed_frames))
        metadata.append('#define SC64_RACE_UNIT_WORD_SPRITE_METADATA 1')
        metadata += [
            'static inline const GbaSpriteWordFrame *sc64_race_unit_words_sprite(unsigned short raw_type, const Sc64SpriteFrame *frame) {',
            '    if (raw_type >= 106 || !frame || sc64_race_unit_sprite(raw_type) != frame) return (const GbaSpriteWordFrame *)0;',
            '    switch (raw_type) {',
        ]
        for guard, packed_frames in packed_groups:
            if guard:
                metadata.append(f'#ifdef {guard}')
            for raw_type, _width, _height, _phase_symbols in packed_frames:
                metadata.append(f'    case {raw_type}: return &sc64_race_unit_words_{raw_type};')
            if guard:
                metadata.append(f'#endif /* {guard} */')
        metadata += [
            '    default: return (const GbaSpriteWordFrame *)0;',
            '    }',
            '}',
            '#endif /* SC64_UNIT_WORD_SPRITES */',
        ]
        lines[-1:-1] = metadata
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text('\n'.join(lines) + '\n', encoding='ascii')


def generate(rom: Path = ROM, out: Path = OUT, header: Path = HEADER,
             remap_path: Path = PALETTE_REMAP, include_t4: bool = True,
             include_t6: bool = False, include_late: bool = False,
             include_zp: bool = False, include_z5: bool = False,
             include_z6: bool = False, include_z5_doodads: bool = False,
             include_p6: bool = False,
             include_z6_buildings: bool = False,
             include_p7: bool = False,
             prepack_unit_words: bool = False) -> dict:
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
    images_dat = base._entry_payload(archive, '00C/000')
    image_iscript = tuple(int.from_bytes(images_dat[IMAGE_COUNT * 10 + i * 4:
                                                    IMAGE_COUNT * 10 + i * 4 + 4],
                                         'little')
                          for i in range(IMAGE_COUNT))
    from tools.sc64_construction_sprites import grp_path, script_index
    iscript_payload = base._entry_payload(archive, '012/002')
    scripts = script_index(iscript_payload)
    image_grp_indices = base._le_array(images_dat, IMAGE_COUNT, 4, 'images.dat GRP')
    image_cache = {}
    init_layer_cache = {}
    from tools.sc64_construction_sprites import (
        animation_instructions, image_animation_starts,
    )

    def init_layer_specs(image_id):
        if image_id in init_layer_cache:
            return init_layer_cache[image_id]
        script_id = image_iscript[image_id]
        if script_id not in scripts:
            init_layer_cache[image_id] = []
            return []
        starts = image_animation_starts(iscript_payload, scripts[script_id])
        start = starts.get('Init')
        if start is None:
            init_layer_cache[image_id] = []
            return []
        # op 8 is imgol (above the current image); op 9 is imgul (below it).
        layers = []
        for opcode, args in animation_instructions(iscript_payload, start):
            if opcode in (8, 9):
                layers.append((args[0], args[1], args[2], opcode == 9))
            elif opcode == 61:  # imgulnextid: underlay image_id + 1, x, y
                layers.append((image_id + 1, args[0], args[1], True))
        init_layer_cache[image_id] = layers
        return layers

    def source_init_frame(image_id, stack=()):
        if image_id in stack:
            raise ValueError(f'cyclic source Init image layers: {stack + (image_id,)}')
        if image_id in image_cache:
            return image_cache[image_id]
        if not 0 <= image_id < IMAGE_COUNT or not image_grp_indices[image_id]:
            raise ValueError(f'source Init layer image {image_id} has no GRP')
        path = grp_path(image_grp_indices[image_id])
        grp_data = archive.read(entries[path])
        grp = base.parse_grp(grp_data)
        frame_index = image_init_frame(image_id, image_iscript,
                                       iscript_payload, scripts)
        if frame_index >= len(grp['frames']):
            raise ValueError(f'image {image_id}: Init frame {frame_index} outside GRP')
        frame = grp['frames'][frame_index]
        if not any(frame['alpha']):
            raise ValueError(f'image {image_id}: source Init frame {frame_index} is transparent')
        layers = []
        for child_id, dx, dy, underlay in init_layer_specs(image_id):
            child_frame = source_init_frame(child_id, stack + (image_id,))
            layers.append((child_frame, dx, dy, underlay))
        composed = compose_init_layers(frame, layers) if layers else frame
        image_cache[image_id] = composed
        return composed

    def compose_frame_bank(image_id, frames):
        layers = [(source_init_frame(child_id, (image_id,)), dx, dy, underlay)
                  for child_id, dx, dy, underlay in init_layer_specs(image_id)]
        return [compose_init_layers(frame, layers) for frame in frames] if layers else frames
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
    race_building_selected = []
    out.mkdir(parents=True, exist_ok=True)
    def load_asset(name, ref, target):
        grp_data = archive.read(entries[ref['bolt_path']])
        grp = base.parse_grp(grp_data)
        frame_index = image_init_frame(ref['image_id'], image_iscript,
                                       iscript_payload, scripts)
        if frame_index >= len(grp['frames']):
            raise ValueError(f'{name}: Init frame {frame_index} outside GRP frame bank')
        frame = source_init_frame(ref['image_id'])
        if not any(frame['alpha']):
            raise ValueError(f'{name}: source Init frame {frame_index} is fully transparent')
        png = out / f'{name}.png'
        base.encode_png(png, frame['width'], frame['height'], base.render_frame(frame, palette))
        target.append({'name': name, 'frame': frame})
        manifest_assets.append({'name': name, **ref, 'sha1': hashlib.sha1(grp_data).hexdigest(),
                                'frame_count': len(grp['frames']),
                                'canvas': [frame['width'], frame['height']],
                                'selected_frame': frame_index, 'frame_rect': [frame['x'], frame['y'],
                                frame['frame_width'], frame['frame_height']],
                                'opaque_pixels': sum(frame['alpha']),
                                'init_layers': [{'image_id': item[0], 'x': item[1],
                                                'y': item[2], 'underlay': item[3]}
                                               for item in init_layer_specs(ref['image_id'])],
                                'preview': png.name})
    for name, ref in refs.items():
        load_asset(name, ref, selected)
    cannon_ref = refs['photon_cannon']
    cannon_grp_data = archive.read(entries[cannon_ref['bolt_path']])
    cannon_grp = base.parse_grp(cannon_grp_data)
    race_building_selected.append({
        'name': 'photon_cannon',
        'frames': compose_frame_bank(cannon_ref['image_id'], cannon_grp['frames']),
        'animations': {
            'gnd_attack_init': image_animation_timeline(
                cannon_ref['image_id'], 'GndAttkInit', image_iscript,
                iscript_payload, scripts),
            'gnd_attack_repeat': image_animation_timeline(
                cannon_ref['image_id'], 'GndAttkRpt', image_iscript,
                iscript_payload, scripts),
        },
    })
    for name, ref in t4_refs.items():
        load_asset(name, ref, t4_selected)
    for name, ref in t4_building_refs.items():
        grp_data = archive.read(entries[ref['bolt_path']])
        grp = base.parse_grp(grp_data)
        frame_list = compose_frame_bank(ref['image_id'], grp['frames'])
        if not frame_list:
            raise ValueError(f'T4 prop raw {ref["unit_id"]} has an empty GRP')
        preview = out / f'{name}.png'
        base.encode_png(preview, frame_list[0]['width'], frame_list[0]['height'], base.render_frame(frame_list[0], palette))
        t4_building_selected.append({'name': name, 'frames': frame_list})
        manifest_assets.append({'name': name, **ref, 'sha1': hashlib.sha1(grp_data).hexdigest(),
                                'frame_count': len(frame_list), 'canvas': [frame_list[0]['width'], frame_list[0]['height']],
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
        frame_list = compose_frame_bank(ref['image_id'], grp['frames'])
        if not frame_list:
            raise ValueError(f'late Terran prop raw {ref["unit_id"]} has an empty GRP')
        preview = out / f'{name}.png'
        base.encode_png(preview, frame_list[0]['width'], frame_list[0]['height'], base.render_frame(frame_list[0], palette))
        late_building_selected.append({'name': name, 'frames': frame_list})
        manifest_assets.append({'name': name, **ref, 'sha1': hashlib.sha1(grp_data).hexdigest(),
                                'frame_count': len(frame_list), 'canvas': [frame_list[0]['width'], frame_list[0]['height']],
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
        frames = compose_frame_bank(ref['image_id'], grp['frames'])
        preview = out / f'{name}.png'
        base.encode_png(preview, frames[0]['width'], frames[0]['height'], base.render_frame(frames[0], palette))
        zp_building_selected.append({'name': name, 'frames': frames})
        manifest_assets.append({'name': name, **ref, 'sha1': hashlib.sha1(grp_data).hexdigest(),
                                'frame_count': len(frames), 'canvas': [frames[0]['width'], frames[0]['height']],
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
        frames = compose_frame_bank(ref['image_id'], grp['frames'])
        preview = out / f'{name}.png'
        base.encode_png(preview, frames[0]['width'], frames[0]['height'], base.render_frame(frames[0], palette))
        z5_building_selected.append({'name': name, 'frames': frames})
        manifest_assets.append({'name': name, **ref, 'sha1': hashlib.sha1(grp_data).hexdigest(),
                                'frame_count': len(frames), 'canvas': [frames[0]['width'], frames[0]['height']],
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
        frames = compose_frame_bank(ref['image_id'], grp['frames'])
        preview = out / f'{name}.png'
        base.encode_png(preview, frames[0]['width'], frames[0]['height'], base.render_frame(frames[0], palette))
        z5_doodad_selected.append({'name': name, 'frames': frames})
        manifest_assets.append({'name': name, **ref, 'sha1': hashlib.sha1(grp_data).hexdigest(),
                                'frame_count': len(frames), 'canvas': [frames[0]['width'], frames[0]['height']],
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
        frames = compose_frame_bank(ref['image_id'], grp['frames'])
        preview = out / f'{name}.png'
        base.encode_png(preview, frames[0]['width'], frames[0]['height'], base.render_frame(frames[0], palette))
        z6_building_selected.append({'name': name, 'frames': frames})
        manifest_assets.append({'name': name, **ref, 'sha1': hashlib.sha1(grp_data).hexdigest(),
                                'frame_count': len(frames), 'canvas': [frames[0]['width'], frames[0]['height']],
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
        frames = compose_frame_bank(ref['image_id'], grp['frames'])
        preview = out / f'{name}.png'
        base.encode_png(preview, frames[0]['width'], frames[0]['height'], base.render_frame(frames[0], palette))
        p6_doodad_selected.append({'name': name, 'frames': frames})
        manifest_assets.append({'name': name, **ref, 'sha1': hashlib.sha1(grp_data).hexdigest(),
                                'frame_count': len(frames), 'canvas': [frames[0]['width'], frames[0]['height']],
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
        frames = compose_frame_bank(ref['image_id'], grp['frames'])
        preview = out / f'{name}.png'
        base.encode_png(preview, frames[0]['width'], frames[0]['height'], base.render_frame(frames[0], palette))
        p7_building_selected.append({'name': name, 'frames': frames})
        manifest_assets.append({'name': name, **ref, 'sha1': hashlib.sha1(grp_data).hexdigest(),
                                'frame_count': len(frames), 'canvas': [frames[0]['width'], frames[0]['height']],
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
                 z6_building_selected, p7_selected, p7_building_selected,
                 race_building_selected, prepack_unit_words)
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
    cannon_script = image_iscript[cannon_ref['image_id']]
    cannon_starts = (image_animation_starts(iscript_payload, scripts[cannon_script])
                     if cannon_script in scripts else {})
    report['photon_cannon_source_animations'] = {
        animation: [args[0] for opcode, args in
                    animation_instructions(iscript_payload, start) if opcode == 0]
        for animation, start in cannon_starts.items()
        if animation in ('Init', 'GndAttkInit', 'GndAttkRpt', 'GndAttkToIdle',
                         'Built', 'IsWorking', 'WorkingToIdle')}
    report['photon_cannon_attack_timelines'] = {
        name: steps for name, steps in race_building_selected[0]['animations'].items()}
    report['photon_cannon_init_layers'] = [
        {'image_id': layer[0], 'x': layer[1], 'y': layer[2],
         'underlay': layer[3]}
        for layer in init_layer_specs(cannon_ref['image_id'])]
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
                        help='include SC64_T6_ENABLED-gated crashed Norad and Control Tower source sprites')
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
    parser.add_argument('--prepack-unit-words', action='store_true',
                        help='emit optional four-phase color/mask words for mapped raw unit IDs below 106')
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
                          include_p7=args.include_p7_actors,
                          prepack_unit_words=args.prepack_unit_words)
    except (OSError, ValueError, RuntimeError, KeyError) as exc:
        print(f'sc64_race_sprites: {exc}', file=sys.stderr)
        return 1
    print(f"Generated {len(report['assets'])} race sprites into {args.header}")
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
