#!/usr/bin/env python3
"""Derive private Ventral Sacs facts from authenticated Z6 scenario data.

The generated header contains numeric values only. It records the source
upgrade allowance, initial level, Overlord availability, and Lair/Hive
availability needed by the optional Z6 transport-tech adapter.
"""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
from pathlib import Path
import tempfile

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_SOURCE_ROOT = ROOT / '.local/sc64/zp-stage/generated/source_maps'
DEFAULT_OUTPUT = ROOT / '.local/sc64/zp-stage/generated/scenarios/sc64_z6_transport_generated.h'
MISSION = 'Z6'
GLOBAL_ID = 17
SOURCE_MAP_PATH = '008/019'
WIDTH = HEIGHT = 128
SOURCE_OWNER = 3
UPGRADE_ID = 24  # Ventral Sacs in the pinned StarCraft upgrade table.
UNIT_BYTES = 36
UPGRADE_COUNT = 46
UPGS_BYTES = 598  # 46 defaults bytes followed by six 46-entry u16 arrays.
UPGR_BYTES = 1748  # non-Brood-War restriction table: 46 upgrades, 12 players.
PUNI_BYTES = 5700  # 12x228 player values, 228 global values, 12x228 defaults.
UNIT_RAW_OVERLORD = 42
UNIT_RAW_HATCHERY = 131
UNIT_RAW_LAIR = 132
UNIT_RAW_HIVE = 133
EXPECTED_OVERLORDS = 3


def _load_scenario_packer():
    spec = importlib.util.spec_from_file_location(
        'sc64_z6_transport_scenario', ROOT / 'tools/sc64_zp_scenario_pack.py')
    if spec is None or spec.loader is None:
        raise RuntimeError('cannot load authenticated Z6 scenario packer')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def _u16(data: bytes, offset: int) -> int:
    return data[offset] | (data[offset + 1] << 8)


def decode_upgs(payload: bytes, upgrade: int = UPGRADE_ID) -> dict[str, int]:
    """Decode the UPGS cost record using the pinned OpenBW field arrays."""
    if not isinstance(payload, bytes) or len(payload) != UPGS_BYTES:
        raise ValueError(f'UPGS must be exactly {UPGS_BYTES} bytes')
    if type(upgrade) is not int or not 0 <= upgrade < UPGRADE_COUNT:
        raise ValueError('upgrade index is outside 0..45')
    default = payload[upgrade]
    if default not in (0, 1):
        raise ValueError('UPGS default selector must be boolean')
    base = UPGRADE_COUNT
    result = {'upgrade_id': upgrade, 'uses_default_settings': default}
    names = ('mineral_cost', 'mineral_factor', 'gas_cost', 'gas_factor',
             'time_cost', 'time_factor')
    for name in names:
        result[name] = _u16(payload, base + upgrade * 2)
        base += UPGRADE_COUNT * 2
    return result


def decode_upgr(payload: bytes, owner: int = SOURCE_OWNER,
                upgrade: int = UPGRADE_ID) -> dict[str, int]:
    """Resolve one UPGR restriction row through player/global defaults."""
    if not isinstance(payload, bytes) or len(payload) != UPGR_BYTES:
        raise ValueError(f'UPGR must be exactly {UPGR_BYTES} bytes')
    if type(owner) is not int or not 0 <= owner < 12:
        raise ValueError('UPGR owner is outside 0..11')
    if type(upgrade) is not int or not 0 <= upgrade < UPGRADE_COUNT:
        raise ValueError('upgrade index is outside 0..45')
    player_values = 12 * UPGRADE_COUNT
    global_base = player_values * 2
    uses_global_base = global_base + UPGRADE_COUNT * 2
    player_index = owner * UPGRADE_COUNT + upgrade
    player_max = payload[player_index]
    player_current = payload[player_values + player_index]
    global_max = payload[global_base + upgrade]
    global_current = payload[global_base + UPGRADE_COUNT + upgrade]
    uses_global = payload[uses_global_base + player_index]
    if uses_global not in (0, 1):
        raise ValueError('UPGR global-default selector must be boolean')
    return {'owner': owner, 'upgrade_id': upgrade,
            'player_max_level': player_max,
            'player_current_level': player_current,
            'global_max_level': global_max,
            'global_current_level': global_current,
            'uses_global_default': uses_global,
            'max_level': global_max if uses_global else player_max,
            'current_level': global_current if uses_global else player_current}


def decode_puni(payload: bytes, owner: int = SOURCE_OWNER,
                 unit_raw: int = UNIT_RAW_OVERLORD) -> dict[str, int]:
    """Resolve the Z6 source owner's PUNI allow/default cells for one unit."""
    if not isinstance(payload, bytes) or len(payload) != PUNI_BYTES:
        raise ValueError(f'PUNI must be exactly {PUNI_BYTES} bytes')
    if type(owner) is not int or not 0 <= owner < 12:
        raise ValueError('PUNI owner is outside 0..11')
    if type(unit_raw) is not int or not 0 <= unit_raw < 228:
        raise ValueError('PUNI unit raw ID is outside 0..227')
    player_index = owner * 228 + unit_raw
    global_base = 12 * 228
    default_base = global_base + 228
    player_available = payload[player_index]
    global_available = payload[global_base + unit_raw]
    use_global = payload[default_base + player_index]
    if any(v not in (0, 1) for v in (player_available, global_available, use_global)):
        raise ValueError('PUNI target cells must be boolean')
    return {'owner': owner, 'unit_raw': unit_raw,
            'player_available': player_available,
            'global_available': global_available,
            'uses_global_default': use_global,
            'available': global_available if use_global else player_available}


def source_unit_census(payload: bytes) -> dict[str, int]:
    if not isinstance(payload, bytes) or not payload or len(payload) % UNIT_BYTES:
        raise ValueError('UNIT must contain complete 36-byte records')
    counts = {UNIT_RAW_OVERLORD: 0, UNIT_RAW_HATCHERY: 0,
              UNIT_RAW_LAIR: 0, UNIT_RAW_HIVE: 0}
    for offset in range(0, len(payload), UNIT_BYTES):
        raw = _u16(payload, offset + 8)
        owner = payload[offset + 16]
        if owner == SOURCE_OWNER and raw in counts:
            counts[raw] += 1
    return {f'owner3_raw{raw}_count': count for raw, count in counts.items()}


def derive_facts(sections: dict[bytes, bytes], scenario: dict) -> dict:
    if scenario.get('mission') != MISSION or (
            scenario.get('global_catalog_id') != GLOBAL_ID or
            scenario.get('source_map_path') != SOURCE_MAP_PATH or
            scenario.get('width') != WIDTH or scenario.get('height') != HEIGHT):
        raise ValueError('Z6 source identity or dimensions differ from pinned mission')
    digest = scenario.get('source_chk_sha256')
    if not isinstance(digest, str) or len(digest) != 64:
        raise ValueError('authenticated Z6 CHK SHA-256 is missing')
    needed = (b'UPGS', b'UPGR', b'PUNI', b'UNIT', b'OWNR')
    missing = [tag.decode('ascii').strip() for tag in needed if tag not in sections]
    if missing:
        raise ValueError(f'Z6 CHK lacks transport source records: {missing}')
    if len(sections[b'OWNR']) < 8 or sections[b'OWNR'][SOURCE_OWNER] != 6:
        raise ValueError('Z6 playable source owner3 is not authenticated human slot')
    costs = decode_upgs(sections[b'UPGS'])
    levels = decode_upgr(sections[b'UPGR'])
    overlord = decode_puni(sections[b'PUNI'], unit_raw=UNIT_RAW_OVERLORD)
    lair = decode_puni(sections[b'PUNI'], unit_raw=UNIT_RAW_LAIR)
    hive = decode_puni(sections[b'PUNI'], unit_raw=UNIT_RAW_HIVE)
    census = source_unit_census(sections[b'UNIT'])
    expected_costs = (1, 200, 0, 200, 0, 2400, 0)
    actual_costs = tuple(costs[key] for key in (
        'uses_default_settings', 'mineral_cost', 'mineral_factor',
        'gas_cost', 'gas_factor', 'time_cost', 'time_factor'))
    if actual_costs != expected_costs:
        raise ValueError('Z6 Ventral Sacs source cost/default record changed')
    if (levels['max_level'], levels['current_level'], levels['uses_global_default']) != (1, 0, 1):
        raise ValueError('Z6 slot3 Ventral Sacs must allow level1 and start at level0')
    if (overlord['available'], lair['available'], hive['available']) != (1, 1, 1):
        raise ValueError('Z6 PUNI must allow Overlords and Lair/Hive for source owner3')
    if (census['owner3_raw42_count'], census['owner3_raw131_count'],
            census['owner3_raw132_count'], census['owner3_raw133_count']) != (3, 1, 0, 0):
        raise ValueError('Z6 starting Overlord/Hatchery/Lair/Hive source census changed')
    return {
        'source_chk_sha256': digest, 'source_owner': SOURCE_OWNER,
        'upgrade_id': UPGRADE_ID, 'source_max_level': levels['max_level'],
        'source_initial_level': levels['current_level'],
        'source_upgrade_uses_global_default': levels['uses_global_default'],
        'uses_default_costs': costs['uses_default_settings'],
        'mineral_cost': costs['mineral_cost'], 'mineral_factor': costs['mineral_factor'],
        'gas_cost': costs['gas_cost'], 'gas_factor': costs['gas_factor'],
        'time_cost_source_frames': costs['time_cost'], 'time_factor': costs['time_factor'],
        'source_overlord_allowed': overlord['available'],
        'source_lair_allowed': lair['available'], 'source_hive_allowed': hive['available'],
        'overlord_raw_type': UNIT_RAW_OVERLORD,
        'hatchery_raw_type': UNIT_RAW_HATCHERY,
        'lair_raw_type': UNIT_RAW_LAIR, 'hive_raw_type': UNIT_RAW_HIVE,
        'starting_overlord_count': census['owner3_raw42_count'],
        'starting_hatchery_count': census['owner3_raw131_count'],
        'starting_lair_count': census['owner3_raw132_count'],
        'starting_hive_count': census['owner3_raw133_count'],
    }


def parse_source(source_root: Path) -> dict:
    packer = _load_scenario_packer()
    scenario = packer.parse_source(source_root, MISSION)
    try:
        chk = (source_root / MISSION.lower() / 'source.chk').read_bytes()
    except OSError as exc:
        raise ValueError(f'cannot reread authenticated Z6 source CHK: {exc}') from exc
    digest = hashlib.sha256(chk).hexdigest()
    if digest != scenario.get('source_chk_sha256'):
        raise ValueError('Z6 source CHK changed between authentication and transport parse')
    facts = derive_facts(packer.parse_chk_sections(chk), scenario)
    return {'mission': MISSION, 'global_catalog_id': GLOBAL_ID,
            'source_map_path': SOURCE_MAP_PATH, **facts}


def emit_header(data: dict) -> str:
    if data.get('mission') != MISSION or data.get('global_catalog_id') != GLOBAL_ID:
        raise ValueError('cannot emit transport facts for a different mission')
    fields = (
        ('SOURCE_OWNER', 'source_owner'), ('UPGRADE_ID', 'upgrade_id'),
        ('SOURCE_MAX_LEVEL', 'source_max_level'), ('SOURCE_INITIAL_LEVEL', 'source_initial_level'),
        ('SOURCE_UPGRADE_USES_GLOBAL_DEFAULT', 'source_upgrade_uses_global_default'),
        ('USES_DEFAULT_COSTS', 'uses_default_costs'), ('MINERAL_COST', 'mineral_cost'),
        ('MINERAL_FACTOR', 'mineral_factor'), ('GAS_COST', 'gas_cost'),
        ('GAS_FACTOR', 'gas_factor'), ('TIME_COST_SOURCE_FRAMES', 'time_cost_source_frames'),
        ('TIME_FACTOR', 'time_factor'), ('SOURCE_OVERLORD_ALLOWED', 'source_overlord_allowed'),
        ('SOURCE_LAIR_ALLOWED', 'source_lair_allowed'), ('SOURCE_HIVE_ALLOWED', 'source_hive_allowed'),
        ('OVERLORD_RAW_TYPE', 'overlord_raw_type'), ('HATCHERY_RAW_TYPE', 'hatchery_raw_type'),
        ('LAIR_RAW_TYPE', 'lair_raw_type'), ('HIVE_RAW_TYPE', 'hive_raw_type'),
        ('STARTING_OVERLORD_COUNT', 'starting_overlord_count'),
        ('STARTING_HATCHERY_COUNT', 'starting_hatchery_count'),
        ('STARTING_LAIR_COUNT', 'starting_lair_count'), ('STARTING_HIVE_COUNT', 'starting_hive_count'),
    )
    out = ['#ifndef SC64_Z6_TRANSPORT_GENERATED_H', '#define SC64_Z6_TRANSPORT_GENERATED_H',
           '/* Authenticated numeric Z6 Ventral Sacs facts; private source-derived data. */',
           f'#define SC64_Z6_TRANSPORT_SOURCE_GLOBAL_ID {GLOBAL_ID}u',
           f'#define SC64_Z6_TRANSPORT_SOURCE_CHK_SHA256 "{data.get("source_chk_sha256", "")}"']
    if len(data.get('source_chk_sha256', '')) != 64:
        raise ValueError('authenticated Z6 source hash is missing')
    for macro, key in fields:
        value = data.get(key)
        if type(value) is not int or value < 0:
            raise ValueError(f'incomplete Z6 transport fact: {key}')
        out.append(f'#define SC64_Z6_TRANSPORT_{macro} {value}u')
    out.extend(['#define SC64_Z6_TRANSPORT_FACTS_COMPLETE 1u', '#endif'])
    return '\n'.join(out) + '\n'


def verify_staged(source_root: Path) -> dict:
    data = parse_source(source_root)
    return {key: data[key] for key in ('mission', 'global_catalog_id', 'source_map_path',
        'source_chk_sha256', 'upgrade_id', 'source_max_level', 'source_initial_level',
        'mineral_cost', 'gas_cost', 'time_cost_source_frames', 'source_overlord_allowed',
        'source_lair_allowed', 'source_hive_allowed', 'starting_overlord_count',
        'starting_hatchery_count', 'starting_lair_count', 'starting_hive_count')}


def generate(source_root: Path, output: Path) -> dict:
    content = emit_header(parse_source(source_root))
    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile('w', encoding='ascii', dir=output.parent,
            prefix=output.name + '.', delete=False) as f:
        tmp = Path(f.name)
        f.write(content)
    tmp.replace(output)
    return verify_staged(source_root)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-root', type=Path, default=DEFAULT_SOURCE_ROOT)
    parser.add_argument('--out', type=Path, default=DEFAULT_OUTPUT)
    args = parser.parse_args()
    try:
        print(generate(args.source_root, args.out))
    except (OSError, ValueError, RuntimeError, KeyError) as exc:
        parser.error(str(exc))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
