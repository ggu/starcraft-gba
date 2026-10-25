#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Extract pinned BWAPI unit and weapon default-table facts to private JSON."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

REFERENCE: Path
OUT: Path
COMMIT = 'd727fed68558c506163048ea889131d8cbb33915'
UNIT_CPP = Path('bwapi/BWAPILIB/Source/UnitType.cpp')
WEAPON_CPP = Path('bwapi/BWAPILIB/Source/WeaponType.cpp')
UNIT_H = Path('bwapi/include/BWAPI/UnitType.h')
UNIT_SIZE_H = Path('bwapi/include/BWAPI/UnitSizeType.h')
WEAPON_H = Path('bwapi/include/BWAPI/WeaponType.h')
DAMAGE_H = Path('bwapi/include/BWAPI/DamageType.h')


def checked_source(path: Path) -> tuple[str, str]:
    full = REFERENCE / path
    data = full.read_bytes()
    return data.decode('utf-8'), hashlib.sha256(data).hexdigest()


def ensure_reference() -> None:
    actual = subprocess.run(
        ['git', '-C', str(REFERENCE), 'rev-parse', 'HEAD'],
        check=True, capture_output=True, text=True).stdout.strip()
    if actual != COMMIT:
        raise SystemExit(f'BWAPI checkout is {actual}; expected pinned {COMMIT}')
    dirty = subprocess.run(
        ['git', '-C', str(REFERENCE), 'status', '--porcelain'],
        check=True, capture_output=True, text=True).stdout
    if dirty:
        raise SystemExit('BWAPI reference checkout has local modifications')


def remove_comments(text: str) -> str:
    """Remove C++ comments while preserving quoted literals and line breaks."""
    out: list[str] = []
    i = 0
    state = 'code'
    while i < len(text):
        c = text[i]
        nxt = text[i + 1] if i + 1 < len(text) else ''
        if state == 'code':
            if c == '"':
                state = 'string'
                out.append(c)
            elif c == '/' and nxt == '/':
                state = 'line'
                out.extend('  ')
                i += 1
            elif c == '/' and nxt == '*':
                state = 'block'
                out.extend('  ')
                i += 1
            else:
                out.append(c)
        elif state == 'string':
            out.append(c)
            if c == '\\' and nxt:
                out.append(nxt)
                i += 1
            elif c == '"':
                state = 'code'
        elif state == 'line':
            if c == '\n':
                out.append(c)
                state = 'code'
            else:
                out.append(' ')
        else:
            if c == '*' and nxt == '/':
                out.extend('  ')
                i += 1
                state = 'code'
            else:
                out.append('\n' if c == '\n' else ' ')
        i += 1
    if state in ('string', 'block'):
        raise ValueError('unterminated C++ string or comment')
    return ''.join(out)


def extract_initializer(text: str, name: str) -> list[str]:
    clean = remove_comments(text)
    match = re.search(r'\b' + re.escape(name) + r'\s*\[[^]]+\]\s*=\s*\{', clean)
    if not match:
        raise ValueError(f'initializer not found: {name}')
    start = match.end() - 1
    depth = 0
    quoted = False
    escaped = False
    end = None
    for index in range(start, len(clean)):
        c = clean[index]
        if quoted:
            if escaped:
                escaped = False
            elif c == '\\':
                escaped = True
            elif c == '"':
                quoted = False
            continue
        if c == '"':
            quoted = True
        elif c == '{':
            depth += 1
        elif c == '}':
            depth -= 1
            if depth == 0:
                end = index
                break
    if end is None:
        raise ValueError(f'unterminated initializer: {name}')
    body = clean[start + 1:end]
    entries: list[str] = []
    token: list[str] = []
    quoted = False
    escaped = False
    nested = 0
    for c in body:
        if quoted:
            token.append(c)
            if escaped:
                escaped = False
            elif c == '\\':
                escaped = True
            elif c == '"':
                quoted = False
        elif c == '"':
            quoted = True
            token.append(c)
        elif c in '({[':
            nested += 1
            token.append(c)
        elif c in ')}]':
            nested -= 1
            token.append(c)
        elif c == ',' and nested == 0:
            value = ''.join(token).strip()
            if value:
                entries.append(value)
            token.clear()
        else:
            token.append(c)
    value = ''.join(token).strip()
    if value:
        entries.append(value)
    return entries


def initializer_line(text: str, name: str) -> int:
    clean = remove_comments(text)
    match = re.search(r'\b' + re.escape(name) + r'\s*\[[^]]+\]\s*=\s*\{', clean)
    if not match:
        raise ValueError(f'initializer not found: {name}')
    return clean.count('\n', 0, match.start()) + 1


def enum_values(header: str, enum_name: str) -> dict[str, int]:
    clean = remove_comments(header)
    match = re.search(r'\benum\s+' + re.escape(enum_name) + r'\s*\{([^}]*)\}', clean, re.S)
    if not match:
        raise ValueError(f'enum not found: {enum_name}')
    values: dict[str, int] = {}
    next_value = 0
    for item in match.group(1).split(','):
        item = item.strip()
        if not item:
            continue
        parsed = re.fullmatch(r'([A-Za-z_]\w*)\s*(?:=\s*(-?\d+))?', item)
        if not parsed:
            raise ValueError(f'unparsed {enum_name} entry: {item!r}')
        name, explicit = parsed.groups()
        value = int(explicit) if explicit is not None else next_value
        if name in values or value in values.values():
            raise ValueError(f'duplicate {enum_name} entry/value: {name}={value}')
        values[name] = value
        next_value = value + 1
    return values


def numeric_rows(text: str, name: str, count: int) -> list[int | float]:
    tokens = extract_initializer(text, name)
    if len(tokens) != count:
        raise ValueError(f'{name}: expected {count} rows, found {len(tokens)}')
    out: list[int | float] = []
    for token in tokens:
        token = token.strip()
        if re.fullmatch(r'-?\d+', token):
            out.append(int(token))
        elif re.fullmatch(r'-?(?:\d+\.\d*|\d*\.\d+)(?:[eE][+-]?\d+)?', token):
            out.append(float(token))
        else:
            raise ValueError(f'{name}: non-numeric value {token!r}')
    return out


def string_rows(text: str, name: str, count: int) -> list[str]:
    tokens = extract_initializer(text, name)
    if len(tokens) != count:
        raise ValueError(f'{name}: expected {count} rows, found {len(tokens)}')
    return [json.loads(token) for token in tokens]


def symbolic_rows(text: str, name: str, count: int, ids: dict[str, int]) -> list[dict]:
    tokens = extract_initializer(text, name)
    if len(tokens) != count:
        raise ValueError(f'{name}: expected {count} rows, found {len(tokens)}')
    rows = []
    for token in tokens:
        symbol = token.strip()
        if symbol not in ids:
            raise ValueError(f'{name}: unresolved enum symbol {symbol!r}')
        rows.append({'name': symbol, 'id': ids[symbol]})
    return rows


def main() -> None:
    ensure_reference()
    unit_src, unit_hash = checked_source(UNIT_CPP)
    weapon_src, weapon_hash = checked_source(WEAPON_CPP)
    unit_h, unit_h_hash = checked_source(UNIT_H)
    unit_size_h, unit_size_h_hash = checked_source(UNIT_SIZE_H)
    weapon_h, weapon_h_hash = checked_source(WEAPON_H)
    damage_h, damage_h_hash = checked_source(DAMAGE_H)

    unit_ids = enum_values(unit_h, 'Enum')
    weapon_ids = enum_values(weapon_h, 'Enum')
    damage_ids = enum_values(damage_h, 'Enum')
    size_ids = enum_values(unit_size_h, 'Enum')
    unit_count = unit_ids['MAX']
    weapon_count = weapon_ids['MAX']
    unit_names = string_rows(unit_src, 'typeNames', unit_count)
    weapon_names = string_rows(weapon_src, 'typeNames', weapon_count)

    unit_fields = {
        'hp': 'defaultMaxHP',
        'shields': 'defaultMaxSP',
        'armor': 'defaultArmorAmount',
        'mineral_cost': 'defaultOreCost',
        'gas_cost': 'defaultGasCost',
        'build_time': 'defaultTimeCost',
        'supply_provided': 'unitSupplyProvided',
        'supply_required': 'unitSupplyRequired',
        'top_speed': 'unitTopSpeeds',
        'ground_weapon_hits': 'groundWeaponHits',
        'air_weapon_hits': 'airWeaponHits',
    }
    units_by_id: dict[int, dict] = {
        idx: {'id': idx, 'name': unit_names[idx]}
        for idx in range(unit_count)
    }
    for field, array in unit_fields.items():
        for idx, value in enumerate(numeric_rows(unit_src, array, unit_count)):
            units_by_id[idx][field] = value
    for key, array in (('ground_weapon', 'groundWeapon'), ('air_weapon', 'airWeapon')):
        for idx, value in enumerate(symbolic_rows(unit_src, array, unit_count, weapon_ids)):
            units_by_id[idx][key] = value
    sizes = symbolic_rows(unit_src, 'unitSize', unit_count, size_ids)
    for idx, value in enumerate(sizes):
        units_by_id[idx]['size'] = value
    building_flags = extract_initializer(unit_src, 'unitFlags')
    if len(building_flags) != unit_count:
        raise ValueError(f'unitFlags: expected {unit_count} rows, found {len(building_flags)}')
    for idx, value in enumerate(building_flags):
        units_by_id[idx]['is_building'] = any(part.strip() == 'Building'
                                              for part in value.split('|'))
    builders = symbolic_rows(unit_src, 'whatBuilds', unit_count, unit_ids)
    for idx, value in enumerate(builders):
        units_by_id[idx]['what_builds'] = value

    weapon_fields = {
        'damage_amount': 'defaultWpnDamageAmt',
        'damage_bonus': 'defaultWpnDamageBonus',
        'damage_factor': 'wpnDamageFactor',
        'damage_cooldown': 'wpnDamageCooldowns',
        'min_range': 'wpnMinRange',
        'max_range': 'wpnMaxRange',
    }
    weapons_by_id: dict[int, dict] = {
        idx: {'id': idx, 'name': weapon_names[idx]}
        for idx in range(weapon_count)
    }
    for field, array in weapon_fields.items():
        for idx, value in enumerate(numeric_rows(weapon_src, array, weapon_count)):
            weapons_by_id[idx][field] = value
    for idx, value in enumerate(symbolic_rows(
            weapon_src, 'damageType', weapon_count, damage_ids)):
        weapons_by_id[idx]['damage_type'] = value

    source_tables = {
        'UnitType.cpp': {
            name: initializer_line(unit_src, name)
            for name in ('typeNames', *unit_fields.values(), 'groundWeapon', 'airWeapon',
                         'unitSize', 'unitFlags', 'whatBuilds')
        },
        'WeaponType.cpp': {
            name: initializer_line(weapon_src, name)
            for name in ('typeNames', *weapon_fields.values(), 'damageType')
        },
    }

    facts = {
        'schema': 1,
        'classification': 'stock_pc_bwapi_default_tables; n64_equivalence_unverified',
        'provenance': {
            'repository': str(REFERENCE),
            'commit': COMMIT,
            'source_files': {
                str(path): {'sha256': digest}
                for path, digest in ((UNIT_CPP, unit_hash), (WEAPON_CPP, weapon_hash),
                                     (UNIT_H, unit_h_hash), (UNIT_SIZE_H, unit_size_h_hash),
                                     (WEAPON_H, weapon_h_hash),
                                     (DAMAGE_H, damage_h_hash))
            },
            'method': 'independent local parser of pinned BWAPI C++ initializer and enum declarations',
            'table_start_lines': source_tables,
            'units': {
                'hp_shields': 'integer default BWAPI hitpoint units as declared',
                'damage': 'BWAPI default damage table values',
                'cooldown': 'BWAPI default damage cooldown table values',
                'ranges': 'BWAPI default weapon min/max range values, in pixels',
                'top_speed': 'BWAPI default topSpeed values, pixels per frame; some are approximations',
                'weapon_hits': 'BWAPI maximum hits per unit for ground/air weapons',
                'cost_time_supply': 'BWAPI default unit tables, unchanged',
                'what_builds': 'BWAPI default whatBuilds raw UnitType ID; used for source constructibility census',
            },
            'documentation_sources': {
                'UnitType.h': 'topSpeed is approximate pixels per frame; maxGroundHits/maxAirHits are per-unit hit multiplicities',
                'WeaponType.h': 'damageCooldown is frames; minRange/maxRange are pixels; damageFactor multiplies weapon damage',
            },
        },
        'unit_count': unit_count,
        'weapon_count': weapon_count,
        'damage_types': [{'id': idx, 'name': name} for name, idx in
                         sorted(damage_ids.items(), key=lambda pair: pair[1])],
        'units': [units_by_id[idx] for idx in sorted(units_by_id)],
        'weapons': [weapons_by_id[idx] for idx in sorted(weapons_by_id)],
    }
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(json.dumps(facts, indent=2, sort_keys=True) + '\n', encoding='utf-8')
    print(f"wrote {facts['unit_count']} units and {facts['weapon_count']} weapons to {OUT}")


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--reference', type=Path, required=True, help='Clean local checkout at the pinned BWAPI revision; nothing is downloaded')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    REFERENCE = args.reference.resolve()
    OUT = args.output.resolve()
    main()
