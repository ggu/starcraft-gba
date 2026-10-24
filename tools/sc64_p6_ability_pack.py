#!/usr/bin/env python3
"""Derive private numeric Psionic Storm facts from authenticated P6 source."""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
from pathlib import Path
import tempfile

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_SOURCE_ROOT = ROOT / '.local/sc64/zp-stage/generated/source_maps'
DEFAULT_OUTPUT = ROOT / '.local/sc64/zp-stage/generated/scenarios/sc64_p6_abilities_generated.h'
MISSION = 'P6'
GLOBAL_ID = 25
SOURCE_MAP_PATH = '008/023'
WIDTH = HEIGHT = 128
UNIT_BYTES = 36
PTEC_BYTES = 912
TECH_COUNT = 24
PLAYER_COUNT = 12
PLAYER_AVAILABLE = 0
PLAYER_RESEARCHED = 288
GLOBAL_AVAILABLE = 576
GLOBAL_RESEARCHED = 600
PLAYER_USE_GLOBAL = 624
STORM_TECH = 19
TASSADAR_RAW = 79
TASSADAR_OWNER = 2
VALID_ENERGY = 0x0008
CLOAKED_FLAG = 0x0004


def _load_packer():
    spec = importlib.util.spec_from_file_location(
        'sc64_p6_ability_scenario', ROOT / 'tools/sc64_zp_scenario_pack.py')
    if spec is None or spec.loader is None:
        raise RuntimeError('cannot load authenticated P6 scenario packer')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def ptec_value(data: bytes, owner: int, tech: int) -> dict[str, int]:
    if not isinstance(data, bytes) or len(data) != PTEC_BYTES:
        raise ValueError(f'PTEC must be exactly {PTEC_BYTES} bytes')
    if not isinstance(owner, int) or not 0 <= owner < PLAYER_COUNT:
        raise ValueError('PTEC owner is outside 0..11')
    if not isinstance(tech, int) or not 0 <= tech < TECH_COUNT:
        raise ValueError('PTEC tech is outside 0..23')
    index = owner * TECH_COUNT + tech
    values = (data[PLAYER_AVAILABLE + index],
              data[PLAYER_RESEARCHED + index],
              data[PLAYER_USE_GLOBAL + index],
              data[GLOBAL_AVAILABLE + tech],
              data[GLOBAL_RESEARCHED + tech])
    if any(value not in (0, 1) for value in values):
        raise ValueError('PTEC target cells must be boolean 0/1')
    pa, pr, use_global, ga, gr = values
    return {'owner': owner, 'tech': tech, 'player_available': pa,
            'player_researched': pr, 'use_global_default': use_global,
            'global_available': ga, 'global_researched': gr,
            'available': ga if use_global else pa,
            'researched': gr if use_global else pr}


def decode_tassadar(data: bytes) -> dict[str, int]:
    if not isinstance(data, bytes) or not data or len(data) % UNIT_BYTES:
        raise ValueError('UNIT must contain complete 36-byte records')
    matches = []
    for offset in range(0, len(data), UNIT_BYTES):
        row = {'raw_type': int.from_bytes(data[offset + 8:offset + 10], 'little'),
               'valid_properties': int.from_bytes(data[offset + 14:offset + 16], 'little'),
               'owner': data[offset + 16], 'energy_percent': data[offset + 19],
               'source_flags': int.from_bytes(data[offset + 26:offset + 28], 'little')}
        if row['raw_type'] == TASSADAR_RAW:
            matches.append(row)
    if len(matches) != 1:
        raise ValueError(f'expected exactly one raw79 Tassadar, found {len(matches)}')
    hero = matches[0]
    if hero['owner'] != TASSADAR_OWNER:
        raise ValueError('raw79 Tassadar source owner differs from owner2')
    if not hero['valid_properties'] & VALID_ENERGY:
        raise ValueError('raw79 UNIT energy property is not valid')
    if hero['energy_percent'] != 100:
        raise ValueError('raw79 UNIT energy percentage is not 100')
    if hero['source_flags'] & CLOAKED_FLAG:
        raise ValueError('raw79 Tassadar is initially cloaked in UNIT flags')
    return hero


def derive_facts(sections: dict[bytes, bytes], scenario: dict) -> dict:
    if scenario.get('mission') != MISSION:
        raise ValueError('ability facts require the P6 scenario')
    if (scenario.get('global_catalog_id') != GLOBAL_ID or
            scenario.get('source_map_path') != SOURCE_MAP_PATH or
            scenario.get('width') != WIDTH or scenario.get('height') != HEIGHT):
        raise ValueError('P6 source identity or dimensions differ from pinned mission')
    digest = scenario.get('source_chk_sha256')
    if not isinstance(digest, str) or len(digest) != 64:
        raise ValueError('authenticated P6 CHK SHA-256 is missing')
    if b'PTEC' not in sections or b'UNIT' not in sections:
        raise ValueError('P6 CHK lacks PTEC or UNIT')
    if len(sections[b'PTEC']) != PTEC_BYTES:
        raise ValueError(f'P6 PTEC section has invalid size {len(sections[b"PTEC"])}')
    hero = decode_tassadar(sections[b'UNIT'])
    storm = ptec_value(sections[b'PTEC'], TASSADAR_OWNER, STORM_TECH)
    if (storm['available'], storm['researched']) != (1, 1):
        raise ValueError('P6 PTEC does not authorize researched Storm for owner2')
    return {'source_chk_sha256': digest, 'source_owner': TASSADAR_OWNER,
            'storm_tech_id': STORM_TECH, 'storm_available': storm['available'],
            'storm_researched': storm['researched'], 'hero_raw_type': TASSADAR_RAW,
            'hero_energy_valid': 1, 'hero_initial_energy_percent': hero['energy_percent'],
            'hero_initial_cloaked': 0}


def parse_source(source_root: Path) -> dict:
    packer = _load_packer()
    scenario = packer.parse_source(source_root, MISSION)
    try:
        chk = (source_root / MISSION.lower() / 'source.chk').read_bytes()
    except OSError as exc:
        raise ValueError(f'cannot reread authenticated P6 CHK: {exc}') from exc
    digest = hashlib.sha256(chk).hexdigest()
    if digest != scenario.get('source_chk_sha256'):
        raise ValueError('P6 source CHK changed between authentication and ability parse')
    facts = derive_facts(packer.parse_chk_sections(chk), scenario)
    return {'mission': MISSION, 'global_catalog_id': GLOBAL_ID,
            'source_map_path': SOURCE_MAP_PATH, 'source_chk_sha256': digest, **facts}


def emit_header(data: dict) -> str:
    if data.get('mission') != MISSION or not data.get('source_chk_sha256'):
        raise ValueError('cannot emit incomplete P6 ability facts')
    fields = (('SOURCE_OWNER', 'source_owner'), ('PSIONIC_STORM_TECH_ID', 'storm_tech_id'),
              ('PSIONIC_STORM_AVAILABLE', 'storm_available'),
              ('PSIONIC_STORM_RESEARCHED', 'storm_researched'),
              ('TASSADAR_RAW_TYPE', 'hero_raw_type'), ('TASSADAR_ENERGY_VALID', 'hero_energy_valid'),
              ('TASSADAR_INITIAL_ENERGY_PERCENT', 'hero_initial_energy_percent'),
              ('TASSADAR_INITIAL_CLOAKED', 'hero_initial_cloaked'))
    out = ['#ifndef SC64_P6_ABILITIES_GENERATED_H', '#define SC64_P6_ABILITIES_GENERATED_H',
           '/* Authenticated numeric P6 ability facts; no source assets or text. */',
           f'#define SC64_P6_ABILITIES_SOURCE_GLOBAL_ID {GLOBAL_ID}u',
           f'#define SC64_P6_ABILITIES_SOURCE_CHK_SHA256 "{data["source_chk_sha256"]}"']
    for macro, key in fields:
        if key not in data or not isinstance(data[key], int):
            raise ValueError(f'incomplete P6 ability fact: {key}')
        out.append(f'#define SC64_P6_ABILITIES_{macro} {data[key]}u')
    out.extend(['#define SC64_P6_ABILITIES_COMPLETE 1u', '#endif'])
    return '\n'.join(out) + '\n'


def verify_staged(source_root: Path) -> dict:
    data = parse_source(source_root)
    return {key: data[key] for key in ('mission', 'global_catalog_id', 'source_map_path',
            'source_chk_sha256', 'storm_available', 'storm_researched',
            'hero_initial_energy_percent', 'hero_initial_cloaked')}


def generate(source_root: Path, output: Path) -> dict:
    content = emit_header(parse_source(source_root))
    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile('w', encoding='ascii', dir=output.parent,
                                     prefix=output.name + '.', delete=False) as stream:
        temporary = Path(stream.name)
        stream.write(content)
    temporary.replace(output)
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
    import json
    print(json.dumps(result, sort_keys=True))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
