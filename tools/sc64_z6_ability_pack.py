#!/usr/bin/env python3
"""Emit private, authenticated numeric Z6 hero-ability facts.

Only source CHK hashes, PTEC values, and numeric detector facts are emitted;
the original map strings, dialogue, and audio are not copied.
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
DEFAULT_OUTPUT = ROOT / '.local/sc64/zp-stage/generated/scenarios/sc64_z6_abilities_generated.h'
MISSION = 'Z6'
GLOBAL_ID = 17
SOURCE_MAP_PATH = '008/019'
MAP_WIDTH = MAP_HEIGHT = 128
UNIT_BYTES = 36
PTEC_BYTES = 912
PTEC_TECH_COUNT = 24
PTEC_PLAYER_COUNT = 12
PTEC_PLAYER_AVAILABLE = 0
PTEC_PLAYER_RESEARCHED = 12 * PTEC_TECH_COUNT
PTEC_GLOBAL_AVAILABLE = 24 * PTEC_TECH_COUNT
PTEC_GLOBAL_RESEARCHED = PTEC_GLOBAL_AVAILABLE + PTEC_TECH_COUNT
PTEC_PLAYER_USE_GLOBAL = PTEC_GLOBAL_RESEARCHED + PTEC_TECH_COUNT
PTEC_CLOAK_ID, PTEC_CONSUME_ID, PTEC_ENSNARE_ID, PTEC_STORM_ID = 10, 16, 17, 19
HERO_RAW_TYPE = 51
HERO_SOURCE_OWNER = 3
UNIT_VALID_ENERGY = 0x0008
UNIT_FLAG_CLOAKED = 0x0004


def _load_scenario_packer():
    path = ROOT / 'tools/sc64_zp_scenario_pack.py'
    spec = importlib.util.spec_from_file_location('sc64_z6_ability_scenario', path)
    if spec is None or spec.loader is None:
        raise RuntimeError('cannot load shared authenticated scenario packer')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def ptec_value(payload: bytes, owner: int, tech: int) -> dict[str, int]:
    """Resolve player/global PTEC arrays with exact bounds and booleans."""
    if not isinstance(payload, bytes) or len(payload) != PTEC_BYTES:
        raise ValueError(f'PTEC must be exactly {PTEC_BYTES} bytes')
    if not isinstance(owner, int) or not 0 <= owner < PTEC_PLAYER_COUNT:
        raise ValueError('PTEC owner is outside 0..11')
    if not isinstance(tech, int) or not 0 <= tech < PTEC_TECH_COUNT:
        raise ValueError('PTEC tech is outside 0..23')
    ix = owner * PTEC_TECH_COUNT + tech
    pa = payload[PTEC_PLAYER_AVAILABLE + ix]
    pr = payload[PTEC_PLAYER_RESEARCHED + ix]
    use_global = payload[PTEC_PLAYER_USE_GLOBAL + ix]
    ga = payload[PTEC_GLOBAL_AVAILABLE + tech]
    gr = payload[PTEC_GLOBAL_RESEARCHED + tech]
    if any(v not in (0, 1) for v in (pa, pr, use_global, ga, gr)):
        raise ValueError('PTEC target cells must be boolean 0/1')
    return {'owner': owner, 'tech': tech, 'player_available': pa,
            'player_researched': pr, 'use_global_default': use_global,
            'global_available': ga, 'global_researched': gr,
            'available': ga if use_global else pa,
            'researched': gr if use_global else pr}


def decode_hero(payload: bytes) -> dict[str, int]:
    """Read the single raw51 UNIT row's valid energy and active cloak state."""
    if not isinstance(payload, bytes) or not payload or len(payload) % UNIT_BYTES:
        raise ValueError('UNIT must contain complete 36-byte records')
    found = []
    for off in range(0, len(payload), UNIT_BYTES):
        raw = int.from_bytes(payload[off + 8:off + 10], 'little')
        if raw != HERO_RAW_TYPE:
            continue
        found.append({'raw_type': raw,
                      'valid_properties': int.from_bytes(payload[off + 14:off + 16], 'little'),
                      'owner': payload[off + 16], 'energy_percent': payload[off + 19],
                      'source_flags': int.from_bytes(payload[off + 26:off + 28], 'little')})
    if len(found) != 1:
        raise ValueError(f'expected exactly one raw51 hero, found {len(found)}')
    hero = found[0]
    if hero['owner'] != HERO_SOURCE_OWNER:
        raise ValueError('raw51 hero source owner differs from owner3')
    if not hero['valid_properties'] & UNIT_VALID_ENERGY:
        raise ValueError('raw51 hero UNIT energy property is not valid')
    if hero['energy_percent'] != 100:
        raise ValueError('raw51 hero UNIT energy percentage is not 100')
    if hero['source_flags'] & UNIT_FLAG_CLOAKED:
        raise ValueError('raw51 hero is initially cloaked in UNIT flags')
    return hero


def derive_facts(sections: dict[bytes, bytes], scenario: dict) -> dict:
    if scenario.get('mission') != MISSION:
        raise ValueError('ability facts require the Z6 scenario')
    if (scenario.get('global_catalog_id') != GLOBAL_ID or
        scenario.get('source_map_path') != SOURCE_MAP_PATH or
        scenario.get('width') != MAP_WIDTH or scenario.get('height') != MAP_HEIGHT):
        raise ValueError('Z6 source identity or dimensions differ from pinned mission')
    digest = scenario.get('source_chk_sha256')
    if not isinstance(digest, str) or len(digest) != 64:
        raise ValueError('authenticated Z6 CHK SHA-256 is missing')
    if b'PTEC' not in sections or b'UNIT' not in sections:
        raise ValueError('Z6 CHK lacks PTEC or UNIT')
    hero = decode_hero(sections[b'UNIT'])
    ptec = sections[b'PTEC']
    cloak = ptec_value(ptec, HERO_SOURCE_OWNER, PTEC_CLOAK_ID)
    consume = ptec_value(ptec, HERO_SOURCE_OWNER, PTEC_CONSUME_ID)
    ensnare = ptec_value(ptec, HERO_SOURCE_OWNER, PTEC_ENSNARE_ID)
    storm = ptec_value(ptec, HERO_SOURCE_OWNER, PTEC_STORM_ID)
    if (cloak['available'], cloak['researched']) != (1, 0):
        raise ValueError('Z6 PTEC must make Personnel Cloaking available but unresearched for owner3')
    if (consume['available'], consume['researched']) != (0, 0):
        raise ValueError('Z6 PTEC must not enable Consume for owner3')
    if (ensnare['available'], ensnare['researched']) != (1, 0):
        raise ValueError('Z6 PTEC must make Ensnare available but unresearched for owner3')
    if (storm['available'], storm['researched']) != (1, 0):
        raise ValueError('Z6 PTEC must make Psionic Storm available but unresearched for owner3')
    return {
        'source_chk_sha256': digest, 'source_owner': HERO_SOURCE_OWNER,
        'cloak_tech_id': PTEC_CLOAK_ID, 'cloak_available': cloak['available'],
        'cloak_researched': cloak['researched'], 'cloak_hero_research_exemption_required': 1,
        'consume_tech_id': PTEC_CONSUME_ID, 'consume_available': consume['available'],
        'consume_researched': consume['researched'],
        'ensnare_tech_id': PTEC_ENSNARE_ID, 'ensnare_available': ensnare['available'],
        'ensnare_researched': ensnare['researched'],
        'storm_tech_id': PTEC_STORM_ID, 'storm_available': storm['available'],
        'storm_researched': storm['researched'],
        'storm_hero_research_exemption_required': 1,
        'hero_raw_type': HERO_RAW_TYPE, 'hero_energy_valid': 1,
        'hero_initial_energy_percent': hero['energy_percent'],
        'hero_initial_cloaked': 0,
        # Pinned UnitType facts: raw84 Observer (9 tiles) and raw162 Photon Cannon
        # (11 tiles). Coordinates/extents are source pixels; detectors are live-world scans.
        'observer_raw_type': 84, 'observer_sight_tiles': 9,
        'photon_cannon_raw_type': 162, 'photon_cannon_sight_tiles': 11,
    }


def parse_source(source_root: Path) -> dict:
    decoder = _load_scenario_packer()
    scenario = decoder.parse_source(source_root, MISSION)
    try:
        chk = (source_root / MISSION.lower() / 'source.chk').read_bytes()
    except OSError as exc:
        raise ValueError(f'cannot reread authenticated Z6 source CHK: {exc}') from exc
    digest = hashlib.sha256(chk).hexdigest()
    if digest != scenario.get('source_chk_sha256'):
        raise ValueError('Z6 source CHK changed between authentication and ability parse')
    sections = decoder.parse_chk_sections(chk)
    facts = derive_facts(sections, scenario)
    return {'mission': MISSION, 'global_catalog_id': GLOBAL_ID,
            'source_map_path': SOURCE_MAP_PATH, **facts}


def emit_header(data: dict) -> str:
    if data.get('mission') != MISSION or not data.get('source_chk_sha256'):
        raise ValueError('cannot emit incomplete Z6 ability facts')
    fields = (
        ('SOURCE_OWNER', 'source_owner'), ('PERSONNEL_CLOAK_TECH_ID', 'cloak_tech_id'),
        ('PERSONNEL_CLOAK_AVAILABLE', 'cloak_available'),
        ('PERSONNEL_CLOAK_RESEARCHED', 'cloak_researched'),
        ('PERSONNEL_CLOAK_HERO_RESEARCH_EXEMPTION', 'cloak_hero_research_exemption_required'),
        ('CONSUME_TECH_ID', 'consume_tech_id'), ('CONSUME_AVAILABLE', 'consume_available'),
        ('CONSUME_RESEARCHED', 'consume_researched'),
        ('ENSNARE_TECH_ID', 'ensnare_tech_id'), ('ENSNARE_AVAILABLE', 'ensnare_available'),
        ('ENSNARE_RESEARCHED', 'ensnare_researched'),
        ('PSIONIC_STORM_TECH_ID', 'storm_tech_id'),
        ('PSIONIC_STORM_AVAILABLE', 'storm_available'),
        ('PSIONIC_STORM_RESEARCHED', 'storm_researched'),
        ('PSIONIC_STORM_HERO_RESEARCH_EXEMPTION', 'storm_hero_research_exemption_required'),
        ('HERO_RAW_TYPE', 'hero_raw_type'), ('HERO_ENERGY_VALID', 'hero_energy_valid'),
        ('HERO_INITIAL_ENERGY_PERCENT', 'hero_initial_energy_percent'),
        ('HERO_INITIAL_CLOAKED', 'hero_initial_cloaked'),
        ('OBSERVER_RAW_TYPE', 'observer_raw_type'), ('OBSERVER_SIGHT_TILES', 'observer_sight_tiles'),
        ('PHOTON_CANNON_RAW_TYPE', 'photon_cannon_raw_type'),
        ('PHOTON_CANNON_SIGHT_TILES', 'photon_cannon_sight_tiles'),
    )
    out = ['#ifndef SC64_Z6_ABILITIES_GENERATED_H', '#define SC64_Z6_ABILITIES_GENERATED_H',
           '/* Authenticated numeric Z6 ability facts; no source assets or text. */',
           f'#define SC64_Z6_ABILITIES_SOURCE_GLOBAL_ID {GLOBAL_ID}u',
           f'#define SC64_Z6_ABILITIES_SOURCE_CHK_SHA256 "{data["source_chk_sha256"]}"']
    for macro, key in fields:
        value = data.get(key)
        if not isinstance(value, int):
            raise ValueError(f'incomplete Z6 ability fact: {key}')
        out.append(f'#define SC64_Z6_ABILITIES_{macro} {value}u')
    out.extend(['#define SC64_Z6_ABILITIES_COMPLETE 1u', '#endif'])
    return '\n'.join(out) + '\n'


def verify_staged(source_root: Path) -> dict:
    data = parse_source(source_root)
    return {k: data[k] for k in ('mission', 'global_catalog_id', 'source_map_path',
        'source_chk_sha256', 'cloak_available', 'cloak_researched',
        'consume_available', 'consume_researched', 'ensnare_available',
        'ensnare_researched', 'storm_available', 'storm_researched',
        'hero_initial_energy_percent', 'hero_initial_cloaked',
        'observer_raw_type', 'observer_sight_tiles', 'photon_cannon_raw_type',
        'photon_cannon_sight_tiles')}


def generate(source_root: Path, output: Path) -> dict:
    content = emit_header(parse_source(source_root))
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
