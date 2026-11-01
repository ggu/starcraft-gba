#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Generate compact C facts tables from sc64_unit_spec_reference.py output."""
import argparse
import json
from pathlib import Path


def emit(facts_path: Path, output: Path) -> None:
    facts = json.loads(facts_path.read_text(encoding='utf-8'))
    units = facts['units']
    weapons = facts['weapons']
    lines = [
        '/* SPDX-License-Identifier: GPL-3.0-or-later */',
        '#ifndef SC64_STOCK_PROFILES_H', '#define SC64_STOCK_PROFILES_H', '',
        '/* Numeric default-table facts extracted independently from pinned BWAPI',
        ' * d727fed68558c506163048ea889131d8cbb33915. Values do not establish',
        ' * native N64 equivalence. Do not copy BWAPI implementation code here.',
        ' * Unit records are indexed by public raw UnitType ID; raw IDs below None',
        ' * (228) are addressable. Q10 speed is source pixels per source frame.',
        ' */',
        'enum { SC64_STOCK_UNIT_LIMIT = 228, SC64_STOCK_UNIT_ROWS = %d,' % len(units),
        '       SC64_STOCK_WEAPON_ROWS = %d };' % len(weapons),
        'enum { SC64_STOCK_F_BUILDING = 1, SC64_STOCK_F_REGENERATES_HP = 2,',
        '       SC64_STOCK_DMG_INDEPENDENT = 0,',
        '       SC64_STOCK_DMG_EXPLOSIVE = 1, SC64_STOCK_DMG_CONCUSSIVE = 2,',
        '       SC64_STOCK_DMG_NORMAL = 3, SC64_STOCK_DMG_IGNORE_ARMOR = 4,',
        '       SC64_STOCK_DMG_NONE = 5, SC64_STOCK_DMG_UNKNOWN = 6 };',
        'typedef struct { unsigned char flags, ground_weapon, air_weapon,',
        '                         ground_hits, air_hits, size;',
        '                 unsigned short armor, minerals, gas, build_frames,',
        '                                supply_required, supply_provided;',
        '                 unsigned int hp, shields, top_speed_q10; } Sc64StockUnitProfile;',
        'typedef struct { unsigned short damage, upgrade_bonus, cooldown,',
        '                                min_range, max_range;',
        '                 unsigned char damage_factor, damage_class; } Sc64StockWeaponProfile;',
        'static const Sc64StockUnitProfile sc64_stock_unit_profiles[SC64_STOCK_UNIT_ROWS] = {',
    ]
    for u in units:
        if not all(type(u.get(key)) is bool for key in ('is_building', 'regenerates_hp')):
            raise ValueError('unit facts require boolean is_building and regenerates_hp; re-extract the pinned reference')
        flags = int(u['is_building']) | (int(u['regenerates_hp']) << 1)
        speed = int(round(float(u['top_speed']) * 1024))
        fields = (flags, u['ground_weapon']['id'], u['air_weapon']['id'],
                  u['ground_weapon_hits'], u['air_weapon_hits'], u['size']['id'],
                  u['armor'], u['mineral_cost'], u['gas_cost'], u['build_time'],
                  u['supply_required'], u['supply_provided'], u['hp'], u['shields'], speed)
        lines.append('    [%d] = { %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d }, /* %s */' %
                     (u['id'], *fields, u['name']))
    lines.extend(['};', 'static const Sc64StockWeaponProfile sc64_stock_weapon_profiles[SC64_STOCK_WEAPON_ROWS] = {'])
    for w in weapons:
        fields = (w['damage_amount'], w['damage_bonus'], w['damage_cooldown'],
                  w['min_range'], w['max_range'], w['damage_factor'], w['damage_type']['id'])
        lines.append('    [%d] = { %d, %d, %d, %d, %d, %d, %d }, /* %s */' %
                     (w['id'], *fields, w['name']))
    lines.extend([
        '};',
        'static inline const Sc64StockUnitProfile *sc64_stock_unit_profile(int raw_id)',
        '{ return raw_id >= 0 && raw_id < SC64_STOCK_UNIT_LIMIT ? &sc64_stock_unit_profiles[raw_id] : 0; }',
        'static inline const Sc64StockUnitProfile *sc64_stock_building_profile(int raw_id)',
        '{ const Sc64StockUnitProfile *p = sc64_stock_unit_profile(raw_id);',
        '  return p && (p->flags & SC64_STOCK_F_BUILDING) ? p : 0; }',
        'static inline const Sc64StockWeaponProfile *sc64_stock_weapon_profile(int id)',
        '{ return id >= 0 && id < SC64_STOCK_WEAPON_ROWS ? &sc64_stock_weapon_profiles[id] : 0; }',
        '/* Fixed-point rate at 1/2 spatial scale and 24/30 source/runtime clock.',
        ' * Fractional integration belongs to the caller. This table adds no actor fields. */',
        'static inline unsigned int sc64_stock_runtime_speed_q10(const Sc64StockUnitProfile *p)',
        '{ return p ? (p->top_speed_q10 * 2u + 2u) / 5u : 0u; }',
        '#endif', '',
    ])
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text('\n'.join(lines), encoding='utf-8')
    print('generated %d raw-unit rows and %d weapon rows (%d bytes): %s' %
          (len(units), len(weapons), output.stat().st_size, output))


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--facts', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    a = p.parse_args()
    emit(a.facts, a.output)
