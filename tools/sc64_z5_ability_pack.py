#!/usr/bin/env python3
"""Derive private, source-checked Z5 hero-ability facts.

The source CHK is authenticated by the shared Z/P scenario packer. This emits
only small numeric facts for the Z5 runtime; no source assets or text are
copied. By default the generated header is written below ignored .local/.
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
DEFAULT_OUTPUT = ROOT / '.local/sc64/zp-stage/generated/scenarios/sc64_z5_abilities_generated.h'
MISSION = 'Z5'
GLOBAL_ID = 16
SOURCE_MAP_PATH = '008/018'
MAP_WIDTH = 128
MAP_HEIGHT = 128
UNIT_BYTES = 36
PTEC_BYTES = 912
PTEC_TECH_COUNT = 24
PTEC_PLAYER_COUNT = 12
PTEC_PLAYER_AVAILABLE = 0
PTEC_PLAYER_RESEARCHED = 12 * PTEC_TECH_COUNT
PTEC_GLOBAL_AVAILABLE = 24 * PTEC_TECH_COUNT
PTEC_GLOBAL_RESEARCHED = PTEC_GLOBAL_AVAILABLE + PTEC_TECH_COUNT
PTEC_PLAYER_USE_GLOBAL = PTEC_GLOBAL_RESEARCHED + PTEC_TECH_COUNT
PTEC_CLOAK_ID = 10
PTEC_CONSUME_ID = 16
PTEC_ENSNARE_ID = 17
PTEC_STORM_ID = 19
HERO_RAW_TYPE = 51
HERO_SOURCE_OWNER = 3
UNIT_VALID_ENERGY = 0x0008
UNIT_FLAG_CLOAKED = 0x0004
EXPECTED_TRAP_TYPES = {203: 9, 209: 10, 211: 5, 213: 1}
EXPECTED_DOOR_TYPES = {205: 2, 206: 4, 207: 4, 208: 6}


def _load_module(name: str, path: Path):
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f'cannot load {path.name}')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def _scenario_packer():
    return _load_module('sc64_z5_ability_scenario', ROOT / 'tools/sc64_zp_scenario_pack.py')


def _doodad_packer():
    return _load_module('sc64_z5_ability_doodads', ROOT / 'tools/sc64_z5_doodad_pack.py')


def ptec_value(payload: bytes, owner: int, tech: int) -> dict[str, int]:
    """Resolve one PTEC cell using player/default/global arrays."""
    if not isinstance(payload, bytes) or len(payload) != PTEC_BYTES:
        raise ValueError(f'PTEC must be exactly {PTEC_BYTES} bytes')
    if not isinstance(owner, int) or not 0 <= owner < PTEC_PLAYER_COUNT:
        raise ValueError('PTEC owner is outside 0..11')
    if not isinstance(tech, int) or not 0 <= tech < PTEC_TECH_COUNT:
        raise ValueError('PTEC tech is outside 0..23')
    player_index = owner * PTEC_TECH_COUNT + tech
    player_available = payload[PTEC_PLAYER_AVAILABLE + player_index]
    player_researched = payload[PTEC_PLAYER_RESEARCHED + player_index]
    use_global = payload[PTEC_PLAYER_USE_GLOBAL + player_index]
    global_available = payload[PTEC_GLOBAL_AVAILABLE + tech]
    global_researched = payload[PTEC_GLOBAL_RESEARCHED + tech]
    values = (player_available, player_researched, use_global,
              global_available, global_researched)
    if any(value not in (0, 1) for value in values):
        raise ValueError('PTEC target cells must be boolean 0/1')
    return {
        'owner': owner,
        'tech': tech,
        'player_available': player_available,
        'player_researched': player_researched,
        'use_global_default': use_global,
        'global_available': global_available,
        'global_researched': global_researched,
        'available': global_available if use_global else player_available,
        'researched': global_researched if use_global else player_researched,
    }


def decode_hero(payload: bytes) -> dict[str, int]:
    """Read the Z5 Infested Kerrigan's UNIT energy-valid and cloak fields."""
    if not isinstance(payload, bytes) or len(payload) == 0 or len(payload) % UNIT_BYTES:
        raise ValueError('UNIT must contain complete 36-byte records')
    matches = []
    for offset in range(0, len(payload), UNIT_BYTES):
        x = int.from_bytes(payload[offset + 4:offset + 6], 'little')
        y = int.from_bytes(payload[offset + 6:offset + 8], 'little')
        raw_type = int.from_bytes(payload[offset + 8:offset + 10], 'little')
        valid_properties = int.from_bytes(payload[offset + 14:offset + 16], 'little')
        owner = payload[offset + 16]
        energy_percent = payload[offset + 19]
        source_flags = int.from_bytes(payload[offset + 26:offset + 28], 'little')
        if raw_type == HERO_RAW_TYPE:
            matches.append({'x': x, 'y': y, 'raw_type': raw_type,
                            'valid_properties': valid_properties,
                            'owner': owner, 'energy_percent': energy_percent,
                            'source_flags': source_flags})
    if len(matches) != 1:
        raise ValueError(f'expected exactly one raw51 hero, found {len(matches)}')
    hero = matches[0]
    if hero['owner'] != HERO_SOURCE_OWNER:
        raise ValueError('raw51 hero source owner differs from owner3')
    if not (hero['valid_properties'] & UNIT_VALID_ENERGY):
        raise ValueError('raw51 hero UNIT energy property is not valid')
    if hero['energy_percent'] != 100:
        raise ValueError('raw51 hero UNIT energy percentage is not 100')
    if hero['source_flags'] & UNIT_FLAG_CLOAKED:
        raise ValueError('raw51 hero is initially cloaked in UNIT flags')
    return hero


def detector_census(payload: bytes, width: int = MAP_WIDTH,
                    height: int = MAP_HEIGHT) -> dict[str, int]:
    """Verify Z5's complete THG2 trap/door inventory and active flags."""
    decoder = _doodad_packer()
    rows = decoder.decode_thg2(payload, width, height)
    unit_rows = [row for row in rows if row['unit_backed']]
    trap_counts = {raw: sum(row['type'] == raw for row in unit_rows)
                   for raw in EXPECTED_TRAP_TYPES}
    door_counts = {raw: sum(row['type'] == raw for row in unit_rows)
                   for raw in EXPECTED_DOOR_TYPES}
    if trap_counts != EXPECTED_TRAP_TYPES:
        raise ValueError(f'Z5 THG2 detector-trap census differs: {trap_counts}')
    if door_counts != EXPECTED_DOOR_TYPES:
        raise ValueError(f'Z5 THG2 non-detector-door census differs: {door_counts}')
    if len(rows) != 71 or len(unit_rows) != 41:
        raise ValueError('Z5 THG2 row or unit-backed count differs from pinned source')
    initial_active = sum(row['initial_enabled'] for row in unit_rows
                         if row['type'] in EXPECTED_TRAP_TYPES)
    if initial_active != 16:
        raise ValueError(f'Z5 THG2 initially enabled detector count differs: {initial_active}')
    doors = sum(row['type'] in EXPECTED_DOOR_TYPES for row in unit_rows)
    if doors != 16:
        raise ValueError(f'Z5 THG2 non-detector door count differs: {doors}')
    return {
        'thg2_rows': len(rows),
        'unit_backed_rows': len(unit_rows),
        'detector_traps': sum(trap_counts.values()),
        'detector_traps_initially_enabled': initial_active,
        'non_detector_doors': doors,
        'floor_missile_traps': trap_counts[203],
        'floor_gun_traps': trap_counts[209],
        'wall_flame_traps': trap_counts[211],
        'right_wall_flame_traps': trap_counts[213],
    }


def derive_facts(sections: dict[bytes, bytes], scenario: dict) -> dict:
    """Validate known Z5 source facts before producing generated macros."""
    if scenario.get('mission') != MISSION:
        raise ValueError('ability facts require the Z5 scenario')
    if (scenario.get('global_catalog_id') != GLOBAL_ID or
            scenario.get('source_map_path') != SOURCE_MAP_PATH or
            scenario.get('width') != MAP_WIDTH or scenario.get('height') != MAP_HEIGHT):
        raise ValueError('Z5 source identity or dimensions differ from pinned mission')
    if not isinstance(scenario.get('source_chk_sha256'), str) or len(scenario['source_chk_sha256']) != 64:
        raise ValueError('authenticated Z5 CHK SHA-256 is missing')
    if b'PTEC' not in sections or b'UNIT' not in sections or b'THG2' not in sections:
        raise ValueError('Z5 CHK lacks PTEC, UNIT, or THG2')
    ptec = sections[b'PTEC']
    if len(ptec) != PTEC_BYTES:
        raise ValueError(f'Z5 PTEC section has invalid size {len(ptec)}')
    hero = decode_hero(sections[b'UNIT'])
    cloak = ptec_value(ptec, HERO_SOURCE_OWNER, PTEC_CLOAK_ID)
    consume = ptec_value(ptec, HERO_SOURCE_OWNER, PTEC_CONSUME_ID)
    ensnare = ptec_value(ptec, HERO_SOURCE_OWNER, PTEC_ENSNARE_ID)
    storm = ptec_value(ptec, HERO_SOURCE_OWNER, PTEC_STORM_ID)
    if (cloak['available'], cloak['researched']) != (1, 0):
        raise ValueError('Z5 PTEC does not enable unresearched Personnel Cloaking for owner3')
    if storm['available'] != 0:
        raise ValueError('Z5 PTEC unexpectedly enables Psionic Storm for owner3')
    if (consume['available'], consume['researched']) != (1, 0):
        raise ValueError('Z5 PTEC does not enable unresearched Consume for owner3')
    if (ensnare['available'], ensnare['researched']) != (1, 0):
        raise ValueError('Z5 PTEC does not enable unresearched Ensnare for owner3')
    trap_facts = detector_census(sections[b'THG2'], MAP_WIDTH, MAP_HEIGHT)
    return {
        'source_chk_sha256': scenario['source_chk_sha256'],
        'source_owner': HERO_SOURCE_OWNER,
        'cloak_tech_id': PTEC_CLOAK_ID,
        'cloak_available': cloak['available'],
        'cloak_researched': cloak['researched'],
        'cloak_hero_research_exemption_required': 1,
        'consume_tech_id': PTEC_CONSUME_ID,
        'consume_available': consume['available'],
        'consume_researched': consume['researched'],
        'consume_hero_research_exemption_required': 1,
        'ensnare_tech_id': PTEC_ENSNARE_ID,
        'ensnare_available': ensnare['available'],
        'ensnare_researched': ensnare['researched'],
        'ensnare_hero_research_exemption_required': 1,
        'ensnare_status_decrements': 75,
        'ensnare_native_frames_per_status_decrement': 8,
        'ensnare_nominal_native_frames': 600,
        'ensnare_project_gba_ticks_30hz': 750,
        'consume_range_source_pixels': 16,
        'consume_energy_cost': 0,
        'consume_energy_gain': 50,
        'consume_max_energy': 250,
        'storm_tech_id': PTEC_STORM_ID,
        'storm_available': storm['available'],
        'storm_researched': storm['researched'],
        'hero_raw_type': HERO_RAW_TYPE,
        'hero_energy_valid': 1,
        'hero_initial_energy_percent': hero['energy_percent'],
        'hero_initial_cloaked': 0,
        **trap_facts,
    }


def parse_source(source_root: Path) -> dict:
    """Authenticate the staged Z5 CHK through the shared packer, then derive facts."""
    decoder = _scenario_packer()
    scenario = decoder.parse_source(source_root, MISSION)
    try:
        chk = (source_root / MISSION.lower() / 'source.chk').read_bytes()
    except OSError as exc:
        raise ValueError(f'cannot reread authenticated Z5 source CHK: {exc}') from exc
    digest = hashlib.sha256(chk).hexdigest()
    if digest != scenario.get('source_chk_sha256'):
        raise ValueError('Z5 source CHK changed between authentication and ability parse')
    sections = decoder.parse_chk_sections(chk)
    facts = derive_facts(sections, scenario)
    return {'mission': MISSION, 'global_catalog_id': GLOBAL_ID,
            'source_map_path': SOURCE_MAP_PATH,
            'source_chk_sha256': digest, **facts}


def emit_header(data: dict) -> str:
    """Emit compact numeric facts; the marker is last, after validated values."""
    if data.get('mission') != MISSION or data.get('source_chk_sha256') is None:
        raise ValueError('cannot emit incomplete Z5 ability facts')
    fields = (
        ('SOURCE_OWNER', 'source_owner'),
        ('PERSONNEL_CLOAK_TECH_ID', 'cloak_tech_id'),
        ('PERSONNEL_CLOAK_AVAILABLE', 'cloak_available'),
        ('PERSONNEL_CLOAK_RESEARCHED', 'cloak_researched'),
        ('PERSONNEL_CLOAK_HERO_RESEARCH_EXEMPTION', 'cloak_hero_research_exemption_required'),
        ('CONSUME_TECH_ID', 'consume_tech_id'),
        ('CONSUME_AVAILABLE', 'consume_available'),
        ('CONSUME_RESEARCHED', 'consume_researched'),
        ('CONSUME_HERO_RESEARCH_EXEMPTION', 'consume_hero_research_exemption_required'),
        ('CONSUME_RANGE_SOURCE_PIXELS', 'consume_range_source_pixels'),
        ('CONSUME_ENERGY_COST', 'consume_energy_cost'),
        ('CONSUME_ENERGY_GAIN', 'consume_energy_gain'),
        ('CONSUME_MAX_ENERGY', 'consume_max_energy'),
        ('ENSNARE_TECH_ID', 'ensnare_tech_id'),
        ('ENSNARE_AVAILABLE', 'ensnare_available'),
        ('ENSNARE_RESEARCHED', 'ensnare_researched'),
        ('ENSNARE_HERO_RESEARCH_EXEMPTION', 'ensnare_hero_research_exemption_required'),
        ('ENSNARE_STATUS_DECREMENTS', 'ensnare_status_decrements'),
        ('ENSNARE_NATIVE_FRAMES_PER_STATUS_DECREMENT', 'ensnare_native_frames_per_status_decrement'),
        ('ENSNARE_NOMINAL_NATIVE_FRAMES', 'ensnare_nominal_native_frames'),
        ('ENSNARE_PROJECT_GBA_TICKS_30HZ', 'ensnare_project_gba_ticks_30hz'),
        ('PSIONIC_STORM_TECH_ID', 'storm_tech_id'),
        ('PSIONIC_STORM_AVAILABLE', 'storm_available'),
        ('PSIONIC_STORM_RESEARCHED', 'storm_researched'),
        ('KERRIGAN_RAW_TYPE', 'hero_raw_type'),
        ('KERRIGAN_ENERGY_VALID', 'hero_energy_valid'),
        ('KERRIGAN_INITIAL_ENERGY_PERCENT', 'hero_initial_energy_percent'),
        ('KERRIGAN_INITIAL_CLOAKED', 'hero_initial_cloaked'),
        ('DETECTOR_TRAP_COUNT', 'detector_traps'),
        ('DETECTOR_TRAP_INITIAL_ACTIVE_COUNT', 'detector_traps_initially_enabled'),
        ('NON_DETECTOR_DOOR_COUNT', 'non_detector_doors'),
        ('FLOOR_MISSILE_TRAP_COUNT', 'floor_missile_traps'),
        ('FLOOR_GUN_TRAP_COUNT', 'floor_gun_traps'),
        ('WALL_FLAME_TRAP_COUNT', 'wall_flame_traps'),
        ('RIGHT_WALL_FLAME_TRAP_COUNT', 'right_wall_flame_traps'),
    )
    out = [
        '#ifndef SC64_Z5_ABILITIES_GENERATED_H',
        '#define SC64_Z5_ABILITIES_GENERATED_H',
        '/* Authenticated numeric Z5 ability facts; no source assets or text. */',
        f'#define SC64_Z5_ABILITIES_SOURCE_GLOBAL_ID {GLOBAL_ID}u',
        f'#define SC64_Z5_ABILITIES_SOURCE_CHK_SHA256 "{data["source_chk_sha256"]}"',
    ]
    for macro, key in fields:
        if key not in data or not isinstance(data[key], int):
            raise ValueError(f'incomplete Z5 ability fact: {key}')
        out.append(f'#define SC64_Z5_ABILITIES_{macro} {data[key]}u')
    out.extend(['#define SC64_Z5_ABILITIES_COMPLETE 1u', '#endif'])
    return '\n'.join(out) + '\n'


def verify_staged(source_root: Path) -> dict:
    data = parse_source(source_root)
    return {key: data[key] for key in (
        'mission', 'global_catalog_id', 'source_map_path', 'source_chk_sha256',
        'cloak_available', 'cloak_researched', 'consume_available',
        'consume_researched', 'ensnare_available', 'ensnare_researched',
        'ensnare_nominal_native_frames', 'ensnare_project_gba_ticks_30hz',
        'consume_range_source_pixels', 'consume_energy_cost',
        'consume_energy_gain', 'consume_max_energy', 'storm_available',
        'hero_initial_energy_percent', 'hero_initial_cloaked',
        'detector_traps', 'detector_traps_initially_enabled',
        'non_detector_doors')}


def generate(source_root: Path, output: Path) -> dict:
    data = parse_source(source_root)
    content = emit_header(data)
    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile('w', encoding='ascii', dir=output.parent,
                                     prefix=output.name + '.', delete=False) as f:
        temp = Path(f.name)
        f.write(content)
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
