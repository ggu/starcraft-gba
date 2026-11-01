#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Full raw-unit/building/weapon table comparison against extracted facts."""
import argparse
import json
from pathlib import Path
import subprocess
import sys
import tempfile


def check(root: Path, facts_path: Path) -> None:
    facts = json.loads(facts_path.read_text(encoding='utf-8'))
    units = {u['id']: u for u in facts['units'] if u['id'] < 228}
    weapons = {w['id']: w for w in facts['weapons']}
    if any(type(u.get(key)) is not bool for u in units.values()
           for key in ('is_building', 'regenerates_hp')):
        raise ValueError('unit facts require boolean is_building and regenerates_hp; re-extract the pinned reference')
    with tempfile.TemporaryDirectory(prefix='sc64-stock-profiles-') as temp:
        binary = Path(temp) / 'dump'
        subprocess.run(['clang', '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror',
                        'tests/sc64_stock_profiles_dump.c', '-o', str(binary)],
                       cwd=root, check=True)
        lines = subprocess.check_output([str(binary)], cwd=root, text=True).splitlines()
        for profile, flags in (('raw20', []), ('raw19', ['-DSC64_T4_RUNTIME'])):
            fixture = Path(temp) / profile
            subprocess.run(['clang', '-std=c11', '-O2', '-Wall', '-Wextra',
                            '-Werror', *flags,
                            'tests/sc64_raynor_stock_fixture.c', '-o', str(fixture)],
                           cwd=root, check=True)
            subprocess.run([str(fixture)], cwd=root, check=True)
    unit_rows, weapon_rows = {}, {}
    for line in lines:
        row = line.split(',')
        if row[0] == 'U':
            unit_rows[int(row[1])] = list(map(int, row[2:]))
        else:
            weapon_rows[int(row[1])] = list(map(int, row[2:]))
    if set(unit_rows) != set(units):
        raise AssertionError(f'raw unit coverage mismatch: {len(unit_rows)} vs {len(units)}')
    if set(weapon_rows) != set(weapons):
        raise AssertionError(f'weapon coverage mismatch: {len(weapon_rows)} vs {len(weapons)}')
    building_count = 0
    for raw_id, u in units.items():
        expected = [int(u['is_building']) | (int(u['regenerates_hp']) << 1), u['ground_weapon']['id'],
                    u['air_weapon']['id'], u['ground_weapon_hits'],
                    u['air_weapon_hits'], u['size']['id'], u['armor'],
                    u['mineral_cost'], u['gas_cost'], u['build_time'],
                    u['supply_required'], u['supply_provided'], u['hp'],
                    u['shields'], round(float(u['top_speed']) * 1024)]
        if unit_rows[raw_id] != expected:
            raise AssertionError(f'raw unit {raw_id} {u["name"]}: '
                                 f'{unit_rows[raw_id]} != {expected}')
        building_count += int(u['is_building'])
    for weapon_id, w in weapons.items():
        expected = [w['damage_amount'], w['damage_bonus'], w['damage_factor'],
                    w['damage_cooldown'], w['min_range'], w['max_range'],
                    w['damage_type']['id']]
        if weapon_rows[weapon_id] != expected:
            raise AssertionError(f'weapon {weapon_id} {w["name"]}: '
                                 f'{weapon_rows[weapon_id]} != {expected}')
    if building_count == 0:
        raise AssertionError('building flags were not extracted')
    sys.path.insert(0,str(root))
    from tools.sc64_construction_sprites import BUILDINGS, MORPH_PROGRAMS
    expected_construction={u['id'] for u in units.values()
                           if u['is_building'] and u['what_builds']['id']<228}
    imported_construction={row[1] for row in
                           (*BUILDINGS.values(),*MORPH_PROGRAMS.values())}
    if expected_construction!=imported_construction:
        raise AssertionError('construction census mismatch: missing %s, extra %s' %
            (sorted(expected_construction-imported_construction),
             sorted(imported_construction-expected_construction)))
    print(f'checked {len(unit_rows)} raw unit IDs ({building_count} buildings) '
          f'and {len(weapon_rows)} weapon rows')
    print(f'checked {len(expected_construction)} constructible building IDs including morphs')


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[1])
    p.add_argument('--facts', type=Path, required=True)
    args = p.parse_args()
    check(args.root.resolve(), args.facts.resolve())
