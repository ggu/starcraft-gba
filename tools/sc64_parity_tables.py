#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Offline parity layer L0: diff the ROM's own rule tables against the engine.

The authenticated StarCraft 64 ROM carries PC-layout units/weapons/flingy/
upgrades/techdata tables (staged by tools/sc64_openbw_oracle_tables.py). The
engine's rule values come from BWAPI-derived and hand-written headers. This
tool reads both and prints one line per differing cell. It changes nothing:
which side is right is an owner decision, and the existing BWAPI-pinned checks
stay as they are.

Engine sources, all read through tests/sc64_parity_tables_probe.c or a pinned
header pattern so the report names the value the engine actually uses:
  * the raw-ID stock tables (sc64_stock_profiles.h, sc64_stock_upgrade_families.h,
    sc64_source_sight_tiles.h) that weapon selection, motion and sight read;
  * race rules per release profile (race_rules.h and the actor-fact headers it
    includes) for cost, supply, HP, shields, armour, build frames and the
    weapon fallback used when no stock weapon is selected;
  * Protoss research quotes (sc64_protoss_research.h), the generic weapons and
    armour research in game.c, and hand-written spell, research and splash
    constants (ENGINE_CONSTANTS below).

Normalisation: turret weapons are read from the subunit row, HP is 8.8 fixed
point, shields count only when the shield-enable column is set, unit speed is
flingy top speed (1/256 px per frame) in the engine's Q10, and every time is
compared in source frames: engine ticks scaled by (x*5+3)/4 are inverted.

Output goes to $SC64_PARITY_HOME (default: .local/parity in the main
checkout), never to a tracked path. Every entry is tagged provenance rom-table.
Per-mission CHK censuses and iscript sequences are not covered yet.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import shlex
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

UNIT_COUNT, WEAPON_COUNT, FLINGY_COUNT = 228, 130, 209
UPGRADE_COUNT, TECH_COUNT = 61, 44
NONE_WEAPON, NONE_UNIT = 130, 228
PROVENANCE = 'rom-table'
SCHEMA = 1

# Release ROMs whose race rules are compared, as the flag variables their ELF
# recipes pass to the compiler (mk/terran-normal, tests/sc64-zerg-late,
# mk/protoss-p10); art-only word flags are left out.
PROFILES = (
    ('terran', ('TERRAN_CARRIER_FLAGS', 'TERRAN_ROM_FLAGS', 'CANONICAL_DEFENSE_FLAGS',
                'CANONICAL_TARGET_INDEX_FLAGS', 'CANONICAL_TARGET_POLICY_FLAGS')),
    ('zerg-z10', ('Z10_FACT_FLAGS', 'Z10_CARRIER_FLAGS', 'Z10_PADDED_IMPACT_FLAGS',
                  'ZERG_Z10_RELEASE_FLAGS', 'CANONICAL_TARGET_INDEX_FLAGS',
                  'CANONICAL_TARGET_POLICY_FLAGS')),
    ('protoss-p10', ('P10_PADDED_IMPACT_FLAGS', 'PROTOSS_P10_FLAGS', 'CANONICAL_DEFENSE_FLAGS',
                     'CANONICAL_TARGET_INDEX_FLAGS', 'CANONICAL_TARGET_POLICY_FLAGS')),
)

# units.dat, PC Brood War column-major layout (228 rows, 19,876 bytes).
# (name, struct format, first row, row count); rows outside [first, first+count)
# have no entry in that column.
UNIT_FIELDS = (
    ('flingy', 'B', 0, 228), ('subunit1', 'H', 0, 228), ('subunit2', 'H', 0, 228),
    ('infestation', 'H', 106, 96), ('construction', 'I', 0, 228),
    ('direction', 'B', 0, 228), ('shield_enable', 'B', 0, 228),
    ('shields', 'H', 0, 228), ('hp', 'I', 0, 228), ('elevation', 'B', 0, 228),
    ('movement_flags', 'B', 0, 228), ('rank', 'B', 0, 228),
    ('ai_comp_idle', 'B', 0, 228), ('ai_human_idle', 'B', 0, 228),
    ('ai_return', 'B', 0, 228), ('ai_attack_unit', 'B', 0, 228),
    ('ai_attack_move', 'B', 0, 228), ('ground_weapon', 'B', 0, 228),
    ('ground_hits', 'B', 0, 228), ('air_weapon', 'B', 0, 228),
    ('air_hits', 'B', 0, 228), ('ai_internal', 'B', 0, 228),
    ('special_flags', 'I', 0, 228), ('acquire_range', 'B', 0, 228),
    ('sight', 'B', 0, 228), ('armor_upgrade', 'B', 0, 228),
    ('size', 'B', 0, 228), ('armor', 'B', 0, 228), ('right_click', 'B', 0, 228),
    ('ready_sound', 'H', 0, 106), ('what_start', 'H', 0, 228),
    ('what_end', 'H', 0, 228), ('piss_start', 'H', 0, 106),
    ('piss_end', 'H', 0, 106), ('yes_start', 'H', 0, 106),
    ('yes_end', 'H', 0, 106), ('placement', 'I', 0, 228),
    ('addon', 'I', 106, 96), ('dimensions', 'Q', 0, 228),
    ('portrait', 'H', 0, 228), ('minerals', 'H', 0, 228), ('gas', 'H', 0, 228),
    ('build_time', 'H', 0, 228), ('requirements', 'H', 0, 228),
    ('group_flags', 'B', 0, 228), ('supply_provided', 'B', 0, 228),
    ('supply_required', 'B', 0, 228), ('space_required', 'B', 0, 228),
    ('space_provided', 'B', 0, 228), ('build_score', 'H', 0, 228),
    ('destroy_score', 'H', 0, 228), ('map_string', 'H', 0, 228),
    ('broodwar', 'B', 0, 228), ('availability', 'H', 0, 228),
)
UPGRADE_FIELDS = (
    ('mineral_base', 'H'), ('mineral_factor', 'H'), ('gas_base', 'H'),
    ('gas_factor', 'H'), ('time_base', 'H'), ('time_factor', 'H'),
    ('unknown', 'H'), ('icon', 'H'), ('label', 'H'), ('race', 'B'),
    ('max_level', 'B'), ('broodwar', 'B'),
)
TECH_FIELDS = (
    ('minerals', 'H'), ('gas', 'H'), ('research_time', 'H'), ('energy', 'H'),
    ('unknown', 'I'), ('icon', 'H'), ('label', 'H'), ('race', 'B'),
    ('unused', 'B'), ('broodwar', 'B'),
)
SIZES = {'units.dat': 19876, 'weapons.dat': 5460, 'flingy.dat': 3135,
         'upgrades.dat': 1281, 'techdata.dat': 836}

# Public UpgradeType / TechType ids (PC numbering, the same rows the ROM uses).
UPGRADE_NAMES = {
    0: 'Terran_Infantry_Armor', 1: 'Terran_Vehicle_Plating', 2: 'Terran_Ship_Plating',
    3: 'Zerg_Carapace', 4: 'Zerg_Flyer_Carapace', 5: 'Protoss_Ground_Armor',
    6: 'Protoss_Air_Armor', 7: 'Terran_Infantry_Weapons', 8: 'Terran_Vehicle_Weapons',
    9: 'Terran_Ship_Weapons', 10: 'Zerg_Melee_Attacks', 11: 'Zerg_Missile_Attacks',
    12: 'Zerg_Flyer_Attacks', 13: 'Protoss_Ground_Weapons', 14: 'Protoss_Air_Weapons',
    15: 'Protoss_Plasma_Shields', 16: 'U_238_Shells', 17: 'Ion_Thrusters',
    19: 'Titan_Reactor', 20: 'Ocular_Implants', 21: 'Moebius_Reactor',
    22: 'Apollo_Reactor', 23: 'Colossus_Reactor', 24: 'Ventral_Sacs',
    25: 'Antennae', 26: 'Pneumatized_Carapace', 27: 'Metabolic_Boost',
    28: 'Adrenal_Glands', 29: 'Muscular_Augments', 30: 'Grooved_Spines',
    31: 'Gamete_Meiosis', 32: 'Metasynaptic_Node', 33: 'Singularity_Charge',
    34: 'Leg_Enhancements', 35: 'Scarab_Damage', 36: 'Reaver_Capacity',
    37: 'Gravitic_Drive', 38: 'Sensor_Array', 39: 'Gravitic_Boosters',
    40: 'Khaydarin_Amulet', 41: 'Apial_Sensors', 42: 'Gravitic_Thrusters',
    43: 'Carrier_Capacity', 44: 'Khaydarin_Core', 47: 'Argus_Jewel',
    49: 'Argus_Talisman', 51: 'Caduceus_Reactor', 52: 'Chitinous_Plating',
    53: 'Anabolic_Synthesis', 54: 'Charon_Boosters',
}
TECH_NAMES = {
    0: 'Stim_Packs', 1: 'Lockdown', 2: 'EMP_Shockwave', 3: 'Spider_Mines',
    4: 'Scanner_Sweep', 5: 'Tank_Siege_Mode', 6: 'Defensive_Matrix',
    7: 'Irradiate', 8: 'Yamato_Gun', 9: 'Cloaking_Field', 10: 'Personnel_Cloaking',
    11: 'Burrowing', 12: 'Infestation', 13: 'Spawn_Broodlings', 14: 'Dark_Swarm',
    15: 'Plague', 16: 'Consume', 17: 'Ensnare', 18: 'Parasite',
    19: 'Psionic_Storm', 20: 'Hallucination', 21: 'Recall', 22: 'Stasis_Field',
    23: 'Archon_Warp', 24: 'Restoration', 25: 'Disruption_Web', 27: 'Mind_Control',
    28: 'Dark_Archon_Meld', 29: 'Feedback', 30: 'Optical_Flare', 31: 'Maelstrom',
    32: 'Lurker_Aspect', 34: 'Healing',
}

# Protoss research families in sc64_protoss_research.h, in enum order.
PROTOSS_FAMILY_UPGRADES = (5, 6, 13, 14, 15, 33)
# game.c start_research(): one generic weapons and one armour line per race.
# The single weapons level and single armour level raise every family of the
# race, so each family row is compared with the one shared job. It is the
# research every release ROM offers a Terran or Zerg commander (campaign and
# skirmish); Protoss producers use the sc64_protoss_research.h families.
GENERIC_RESEARCH = {
    'terran': {'weapons': (7, 8, 9), 'armor': (0, 1, 2)},
    'zerg': {'weapons': (10, 11, 12), 'armor': (3, 4)},
}

# Hand-written engine constants with no probe-callable table. Each is read from
# its header by pattern so a moved or edited value is noticed, never assumed.
#   (table, row, column, header, pattern, group, scale)
# A value is int(group) * scale; scale converts Q8 energy or ticks as noted.
ENGINE_CONSTANTS = (
    ('weapons', 27, 'splash_inner', 'starcraft/sc64_siege_splash.h', r'if\(distance<=(\d+)\)\{\*tier=SC64_SIEGE_SPLASH_INNER', 1, 1),
    ('weapons', 27, 'splash_medium', 'starcraft/sc64_siege_splash.h', r'if\(distance<=(\d+)\)\{\*tier=SC64_SIEGE_SPLASH_MIDDLE', 1, 1),
    ('weapons', 27, 'splash_outer', 'starcraft/sc64_siege_splash.h', r'distance>(\d+)\)return 0;', 1, 1),
    ('weapons', 28, 'splash_inner', 'starcraft/sc64_siege_splash.h', r'if\(distance<=(\d+)\)\{\*tier=SC64_SIEGE_SPLASH_INNER', 1, 1),
    ('weapons', 28, 'splash_medium', 'starcraft/sc64_siege_splash.h', r'if\(distance<=(\d+)\)\{\*tier=SC64_SIEGE_SPLASH_MIDDLE', 1, 1),
    ('weapons', 28, 'splash_outer', 'starcraft/sc64_siege_splash.h', r'distance>(\d+)\)return 0;', 1, 1),
    # The storm adapters test the GBA-pixel constants; x2 is source pixels.
    ('weapons', 84, 'splash_inner', 'starcraft/sc64_psionic_storm.h', r'SC64_STORM_RADIUS_GBA_PX = (\d+)', 1, 2),
    ('weapons', 84, 'splash_medium', 'starcraft/sc64_psionic_storm.h', r'SC64_STORM_RADIUS_GBA_PX = (\d+)', 1, 2),
    ('weapons', 84, 'splash_outer', 'starcraft/sc64_psionic_storm.h', r'SC64_STORM_RADIUS_GBA_PX = (\d+)', 1, 2),
    ('weapons', 84, 'spell_damage', 'starcraft/sc64_psionic_storm.h', r'SC64_STORM_DAMAGE_PER_PULSE = (\d+)', 1, 1),
    ('weapons', 70, 'splash_inner', 'starcraft/sc64_p7_archon_rules.h', r'SC64_P7_ARCHON_INNER_SOURCE_PX = (\d+)', 1, 1),
    ('weapons', 70, 'splash_medium', 'starcraft/sc64_p7_archon_rules.h', r'SC64_P7_ARCHON_MIDDLE_SOURCE_PX = (\d+)', 1, 1),
    ('weapons', 70, 'splash_outer', 'starcraft/sc64_p7_archon_rules.h', r'SC64_P7_ARCHON_OUTER_SOURCE_PX = (\d+)', 1, 1),
    ('weapons', 54, 'splash_inner', 'starcraft/sc64_p6_combat.h', r'SC64_P6_EXPLOSION_INNER_SOURCE_PX = (\d+)', 1, 1),
    ('weapons', 54, 'splash_medium', 'starcraft/sc64_p6_combat.h', r'SC64_P6_EXPLOSION_MIDDLE_SOURCE_PX = (\d+)', 1, 1),
    ('weapons', 54, 'splash_outer', 'starcraft/sc64_p6_combat.h', r'SC64_P6_EXPLOSION_OUTER_SOURCE_PX = (\d+)', 1, 1),
    ('weapons', 6, 'splash_inner', 'starcraft/sc64_t5_mine_adapter.h', r'SC64_T5_MINE_INNER_SPLASH_SOURCE_PX = (\d+)', 1, 1),
    ('weapons', 6, 'splash_medium', 'starcraft/sc64_t5_mine_adapter.h', r'SC64_T5_MINE_MID_SPLASH_SOURCE_PX = (\d+)', 1, 1),
    ('weapons', 6, 'splash_outer', 'starcraft/sc64_t5_mine_adapter.h', r'SC64_T5_MINE_OUTER_SPLASH_SOURCE_PX = (\d+)', 1, 1),
    ('weapons', 82, 'splash_inner', 'starcraft/sc64_paid_scarab.h', r'distance > (\d+)\) continue;\s*tier = distance <= (\d+) \? 1 : distance <= (\d+)', 2, 1),
    ('weapons', 82, 'splash_medium', 'starcraft/sc64_paid_scarab.h', r'distance > (\d+)\) continue;\s*tier = distance <= (\d+) \? 1 : distance <= (\d+)', 3, 1),
    ('weapons', 82, 'splash_outer', 'starcraft/sc64_paid_scarab.h', r'distance > (\d+)\) continue;\s*tier = distance <= (\d+) \? 1 : distance <= (\d+)', 1, 1),
    ('weapons', 31, 'splash_inner', 'starcraft/sc64_nuke_rules.h', r'SC64_NUKE_RADIUS_INNER = (\d+)', 1, 1),
    ('weapons', 31, 'splash_medium', 'starcraft/sc64_nuke_rules.h', r'SC64_NUKE_RADIUS_MIDDLE = (\d+)', 1, 1),
    ('weapons', 31, 'splash_outer', 'starcraft/sc64_nuke_rules.h', r'SC64_NUKE_RADIUS_OUTER = (\d+)', 1, 1),
    # Dedicated attack handlers that game.c runs instead of the stock weapon.
    ('weapons', 70, 'damage', 'starcraft/sc64_p7_archon_rules.h', r'SC64_P7_ARCHON_BASE_DAMAGE = (\d+)', 1, 1),
    ('weapons', 70, 'bonus', 'starcraft/sc64_p7_archon_rules.h', r'SC64_P7_ARCHON_UPGRADE_DAMAGE = (\d+)', 1, 1),
    ('weapons', 70, 'max_range', 'starcraft/sc64_p7_archon_rules.h', r'SC64_P7_ARCHON_RANGE_SOURCE_PX = (\d+)', 1, 1),
    ('weapons', 70, 'cooldown', 'starcraft/sc64_p7_archon_rules.h', r'SC64_P7_ARCHON_COOLDOWN_GBA_TICKS = (\d+)', 1, 'ticks'),
    ('weapons', 6, 'damage', 'starcraft/sc64_t5_mine_adapter.h', r'SC64_T5_MINE_EXPLOSION_DAMAGE = (\d+)', 1, 1),
    ('weapons', 54, 'damage', 'starcraft/sc64_p6_combat.h', r'SC64_P6_EXPLOSION_DAMAGE = (\d+)', 1, 1),
    ('weapons', 30, 'damage', 'starcraft/sc64_battlecruiser_yamato_projectile.h', r'hit_target_impl\(target,(\d+),sc64_race_unit_from_raw\(12\)', 1, 1),
    ('weapons', 79, 'damage', 'starcraft/sc64_p7_projectile_adapter.h', r'int damage=(\d+)\+\(owner==SIDE\?weapons_level:0\);', 1, 1),
    ('weapons', 84, 'max_range', 'starcraft/sc64_psionic_storm.h', r'SC64_STORM_CAST_RANGE_GBA_PX = (\d+)', 1, 2),
    ('techdata', 19, 'energy', 'starcraft/sc64_psionic_storm.h', r'SC64_STORM_ENERGY_COST = (\d+)', 1, 1),
    ('techdata', 17, 'energy', 'starcraft/sc64_z5_ensnare.h', r'SC64_Z5_ENSNARE_COST_Q8 = (\d+) \* 256', 1, 1),
    ('techdata', 16, 'energy', 'starcraft/sc64_z5_consume.h', r'SC64_Z5_CONSUME_ENERGY_COST = (\d+)', 1, 1),
    ('techdata', 10, 'energy', 'starcraft/sc64_z5_cloak.h', r'SC64_Z5_CLOAK_ACTIVATION_COST_Q8 = (\d+) \* 256', 1, 1),
    ('techdata', 8, 'energy', 'starcraft/sc64_battlecruiser_yamato.h', r'q->carry-=(\d+)\*256', 1, 1),
    ('techdata', 8, 'minerals', 'starcraft/sc64_battlecruiser_rules.h', r'research_cost\(unsigned kind\)\s*\{ return kind == SC64_BC_TECH_YAMATO \? (\d+)u', 1, 1),
    ('techdata', 8, 'gas', 'starcraft/sc64_battlecruiser_rules.h', r'research_cost\(unsigned kind\)\s*\{ return kind == SC64_BC_TECH_YAMATO \? (\d+)u', 1, 1),
    ('techdata', 8, 'research_time', 'starcraft/sc64_battlecruiser_rules.h', r'research_duration\(unsigned kind\)\s*\{ return kind == SC64_BC_TECH_YAMATO \? (\d+)u', 1, 1),
    ('upgrades', 23, 'mineral_base', 'starcraft/sc64_battlecruiser_rules.h', r'research_cost\(unsigned kind\)\s*\{ return kind == SC64_BC_TECH_YAMATO \? \d+u : kind == SC64_BC_TECH_REACTOR \? (\d+)u', 1, 1),
    ('upgrades', 23, 'gas_base', 'starcraft/sc64_battlecruiser_rules.h', r'research_cost\(unsigned kind\)\s*\{ return kind == SC64_BC_TECH_YAMATO \? \d+u : kind == SC64_BC_TECH_REACTOR \? (\d+)u', 1, 1),
    ('upgrades', 23, 'time_base', 'starcraft/sc64_battlecruiser_rules.h', r'research_duration\(unsigned kind\)\s*\{ return kind == SC64_BC_TECH_YAMATO \? \d+u : kind == SC64_BC_TECH_REACTOR \? (\d+)u', 1, 1),
    ('upgrades', 24, 'mineral_base', 'starcraft/sc64_skirmish_transport_tech.h', r'SC64_SK_TRANSPORT_MINERALS = (\d+)', 1, 1),
    ('upgrades', 24, 'gas_base', 'starcraft/sc64_skirmish_transport_tech.h', r'SC64_SK_TRANSPORT_GAS = (\d+)', 1, 1),
    ('upgrades', 24, 'time_base', 'starcraft/sc64_skirmish_transport_tech.h', r'SC64_SK_TRANSPORT_RESEARCH_TICKS = (\d+)', 1, 'ticks'),
)
GENERIC_RESEARCH_PATTERNS = {
    'cost': r'if \(minerals < (\d+) \|\| gas < (\d+)\)',
    'weapons_ticks': r'weapons_research && \+\+weapons_research > (\d+)',
    'armor_ticks': r'armor_research && \+\+armor_research > (\d+)',
    'max_level': r'if \(level >= (\d+) \|\| \*progress\)',
}


class InputsMissing(Exception):
    """Private ROM inputs are not present; the caller reports a clean skip."""


# --------------------------------------------------------------------------
# Table parsing and normalisation (pure; exercised by the asset-free test).

def column_major(data: bytes, count: int, fields, label: str) -> dict:
    """Read a column-major table into {field: [value per row]}."""
    out = {}
    cursor = 0
    for spec in fields:
        name, fmt = spec[0], spec[1]
        first, rows = (spec[2], spec[3]) if len(spec) > 2 else (0, count)
        size = struct.calcsize('<' + fmt)
        if cursor + rows * size > len(data):
            raise ValueError(f'{label}.{name} extends beyond the table')
        values = [None] * count
        for i in range(rows):
            values[first + i] = struct.unpack_from('<' + fmt, data, cursor + i * size)[0]
        out[name] = values
        cursor += rows * size
    if cursor != len(data):
        raise ValueError(f'{label}: parsed {cursor} bytes of {len(data)}')
    return out


def parse_rom_tables(tables: dict) -> dict:
    """Parse the authenticated tables into normalised per-row records."""
    from tools.sc64_weapon_source_audit import (FLINGY_FIELDS, WEAPON_FIELDS,
                                                parse_columns)
    for name, size in SIZES.items():
        if len(tables[name]) != size:
            raise ValueError(f'{name} is {len(tables[name])} bytes, expected {size}')
    units = column_major(tables['units.dat'], UNIT_COUNT, UNIT_FIELDS, 'units')
    weapons = parse_columns(tables['weapons.dat'], WEAPON_COUNT, WEAPON_FIELDS, 'weapons')
    flingy = parse_columns(tables['flingy.dat'], FLINGY_COUNT, FLINGY_FIELDS, 'flingy')
    upgrades = column_major(tables['upgrades.dat'], UPGRADE_COUNT, UPGRADE_FIELDS, 'upgrades')
    techdata = column_major(tables['techdata.dat'], TECH_COUNT, TECH_FIELDS, 'techdata')
    return {
        'units': [normalize_unit(units, flingy, i) for i in range(UNIT_COUNT)],
        'weapons': [{k: weapons[k][i] for k in weapons} for i in range(WEAPON_COUNT)],
        'upgrades': [{k: upgrades[k][i] for k in upgrades} for i in range(UPGRADE_COUNT)],
        'techdata': [{k: techdata[k][i] for k in techdata} for i in range(TECH_COUNT)],
    }


def normalize_unit(units: dict, flingy: dict, raw: int) -> dict:
    """One units.dat row in the engine's terms, with the normalisation noted."""
    row = {k: units[k][raw] for k in units}
    notes = []
    hp_raw = row['hp']
    row['hp_raw'] = hp_raw
    row['hp'] = hp_raw >> 8
    if hp_raw & 0xFF:
        notes.append(f'hp is 8.8 fixed point with fraction {hp_raw & 0xFF}/256')
    row['shields_raw'] = row['shields']
    row['shields'] = row['shields'] if row['shield_enable'] else 0
    if not row['shield_enable'] and row['shields_raw']:
        notes.append(f'shield column {row["shields_raw"]} is disabled')
    row['building'] = row['special_flags'] & 1
    # Turreted units keep their weapons on the subunit row in the ROM.
    sub = row['subunit1']
    if (row['ground_weapon'] == NONE_WEAPON and row['air_weapon'] == NONE_WEAPON
            and sub < UNIT_COUNT
            and (units['ground_weapon'][sub] != NONE_WEAPON or units['air_weapon'][sub] != NONE_WEAPON)):
        for key in ('ground_weapon', 'ground_hits', 'air_weapon', 'air_hits'):
            row[key] = units[key][sub]
        notes.append(f'weapons read from turret subunit {sub}')
    fid = row['flingy']
    row['flingy_top_speed'] = flingy['top_speed'][fid]
    row['movement_control'] = flingy['movement_control'][fid]
    row['top_speed_q10'] = flingy['top_speed'][fid] * 4  # 1/256 px -> 1/1024 px
    row['notes'] = notes
    return row


def ticks_to_source_frames(ticks: int) -> int | None:
    """Invert the engine's (x*5+3)//4 scaling; None if no source value maps."""
    for source in (ticks * 4 // 5, ticks * 4 // 5 + 1):
        if (source * 5 + 3) // 4 == ticks:
            return source
    return None


# --------------------------------------------------------------------------
# Diffing (pure).

# Diff classes. 'gameplay' cells are value differences on rows the engine
# uses as written. 'policy' cells are the engine's authored zero cost on rows
# it never offers for production (heroes, critters, doors, mode switches), or
# the ROM's 1-mineral/1-frame placeholder on such rows. Both are reported.
GAMEPLAY, POLICY = 'gameplay', 'policy'
COST_COLUMNS = ('minerals', 'gas', 'build_frames', 'supply')
# Units whose attack lives on the unit they launch (weapon on the launched row).
LAUNCHED_WEAPON = {83: 85, 81: 85}  # Reaver, Warbringer -> Scarab
# Raw units whose attack a dedicated handler fires instead of the race rules,
# with the defines that compile the handler in (any one suffices). Their weapon
# values are compared in weapons.engine; a profile without the handler is
# checked like any other unit.
DEDICATED_ATTACK = {
    13: ('sc64_t5_mine_adapter.h', ('SC64_T5_RUNTIME',)),
    50: ('sc64_p6_combat_adapter.h', ('SC64_P6_COMBAT_ENABLED',)),
    # game.c includes the projectile adapter under SC64_P7_BATTERY_ENABLED or
    # SC64_CUSTOM_CARRIER_PROBE, which it defines from either Carrier probe.
    73: ('sc64_p7_projectile_adapter.h', ('SC64_P7_BATTERY_ENABLED', 'SC64_CUSTOM_CARRIER_PROBE',
                                          'SC64_TERRAN_CARRIER_PROBE', 'SC64_ZERG_CARRIER_PROBE')),
    85: ('sc64_paid_scarab.h', ('SC64_PAID_SCARAB',)),
}


def cell(table, row, name, column, rom, engine, source, profiles=None, note=None, cls=GAMEPLAY):
    entry = {'provenance': PROVENANCE, 'table': table, 'row': row, 'name': name,
             'column': column, 'rom': rom, 'engine': engine, 'engine_source': source,
             'class': cls}
    if profiles:
        entry['profiles'] = sorted(profiles)
    if note:
        entry['note'] = note
    return entry


UNIT_STOCK_COLUMNS = (
    ('building', 'building'), ('ground_weapon', 'ground_weapon'),
    ('air_weapon', 'air_weapon'), ('ground_hits', 'ground_hits'),
    ('air_hits', 'air_hits'), ('size', 'size'), ('armor', 'armor'),
    ('minerals', 'minerals'), ('gas', 'gas'), ('build_frames', 'build_time'),
    ('supply_required', 'supply_required'), ('supply_provided', 'supply_provided'),
    ('hp', 'hp'), ('shields', 'shields'),
)
WEAPON_STOCK_COLUMNS = (
    ('damage', 'damage'), ('upgrade_bonus', 'bonus'), ('damage_factor', 'count'),
    ('cooldown', 'cooldown'), ('min_range', 'min_range'),
    ('max_range', 'max_range'), ('damage_class', 'damage_type'),
)
SPEED_ROUNDING_Q10 = 6  # BWAPI stores topSpeed to two decimals (<= 5.12 Q10)


def unit_note(rom_unit: dict, column: str) -> str | None:
    """The normalisation notes that bear on one compared column."""
    keys = {'hp': ('hp is',), 'shields': ('shield',),
            'ground_weapon': ('turret',), 'air_weapon': ('turret',),
            'ground_hits': ('turret',), 'air_hits': ('turret',)}.get(column, ())
    if column.startswith(('ground_', 'air_')):
        keys = ('turret',)
    notes = [n for n in rom_unit['notes'] if n.startswith(keys)] if keys else []
    return '; '.join(notes) or None


def diff_stock(rom: dict, engine: dict, unit_names: dict, weapon_names: dict) -> tuple[list, list, list]:
    """Diff the raw-ID stock tables.

    Returns (diffs, speed rounding notices, iscript-speed notices)."""
    diffs, rounding, iscript_speed = [], [], []
    src = 'starcraft/sc64_stock_profiles.h'
    for raw, r in enumerate(rom['units']):
        e = engine['stock_units'].get(raw)
        name = unit_names.get(raw, f'unit {raw}')
        if e is None:
            diffs.append(cell('units.stock', raw, name, 'row', 'present', 'absent', src))
            continue
        for ecol, rcol in UNIT_STOCK_COLUMNS:
            if e[ecol] != r[rcol]:
                diffs.append(cell('units.stock', raw, name, ecol, r[rcol], e[ecol], src,
                                  note=unit_note(r, ecol)))
        delta = e['top_speed_q10'] - r['top_speed_q10']
        if delta and ((r['building'] and r['movement_control'] != 1) or name.startswith('Unused_')):
            # A ROM row that never moves has no comparable speed: a building
            # whose flingy is not partially mobile (only lift-off buildings
            # are), or an unused placeholder row.
            iscript_speed.append(cell('units.stock', raw, name, 'top_speed_q10', r['top_speed_q10'],
                                      e['top_speed_q10'], src, note='ROM row never moves'))
        elif delta and r['movement_control'] == 2:
            # Iscript-driven movers take their rate from walk-animation move
            # opcodes; flingy.dat holds a placeholder. Not a comparable cell.
            iscript_speed.append(cell('units.stock', raw, name, 'top_speed_q10', r['top_speed_q10'],
                                      e['top_speed_q10'], src,
                                      note='flingy movement control is iscript; rate comes from iscript move opcodes'))
        elif delta:
            entry = cell('units.stock', raw, name, 'top_speed_q10', r['top_speed_q10'],
                         e['top_speed_q10'], src,
                         note='flingy movement control is partially mobile' if r['movement_control'] == 1 else None)
            (rounding if abs(delta) <= SPEED_ROUNDING_Q10 else diffs).append(entry)
        if raw in engine['sight_tiles'] and engine['sight_tiles'][raw] != r['sight']:
            diffs.append(cell('units.stock', raw, name, 'sight_tiles', r['sight'],
                              engine['sight_tiles'][raw], 'starcraft/sc64_source_sight_tiles.h'))
        if raw in engine['armor_upgrade'] and engine['armor_upgrade'][raw] != r['armor_upgrade']:
            diffs.append(cell('units.stock', raw, name, 'armor_upgrade', r['armor_upgrade'],
                              engine['armor_upgrade'][raw], 'starcraft/sc64_stock_upgrade_families.h'))
    for wid, w in enumerate(rom['weapons']):
        e = engine['stock_weapons'].get(wid)
        name = weapon_names.get(wid, f'weapon {wid}')
        if e is None:
            diffs.append(cell('weapons.stock', wid, name, 'row', 'present', 'absent', src))
            continue
        for ecol, rcol in WEAPON_STOCK_COLUMNS:
            if e[ecol] != w[rcol]:
                diffs.append(cell('weapons.stock', wid, name, ecol, w[rcol], e[ecol], src))
        if wid in engine['weapon_upgrade'] and engine['weapon_upgrade'][wid] != w['upgrade']:
            diffs.append(cell('weapons.stock', wid, name, 'upgrade', w['upgrade'],
                              engine['weapon_upgrade'][wid], 'starcraft/sc64_stock_upgrade_families.h'))
    return diffs, rounding, iscript_speed


RULE_UNIT_COLUMNS = (
    ('minerals', 'minerals'), ('gas', 'gas'), ('build_frames', 'build_time'),
    ('supply', 'supply_required'), ('supply_provided', 'supply_provided'),
    ('hp', 'hp'), ('shields', 'shields'), ('armor', 'armor'),
)
RULE_BUILDING_COLUMNS = (
    ('minerals', 'minerals'), ('gas', 'gas'), ('build_frames', 'build_time'),
    ('supply_provided', 'supply_provided'), ('hp', 'hp'), ('shields', 'shields'),
    ('armor', 'armor'),
)


def diff_rules(rom: dict, profiles: dict, unit_names: dict) -> tuple[list, set]:
    """Diff race rules per profile, merging identical cells across profiles.

    Returns (diffs, raw ids the race rules cover in any profile)."""
    merged = {}
    covered = set()

    def add(table, raw, column, rom_value, engine_value, source, profile, enum, cls=GAMEPLAY):
        key = (table, raw, column, rom_value, engine_value)
        entry = merged.setdefault(key, {'profiles': set(), 'enums': set(), 'source': source,
                                        'class': cls})
        entry['profiles'].add(profile)
        entry['enums'].add(enum)

    def rule_class(rule, column, rom_value):
        unpriced = not (rule['minerals'] or rule['gas'] or rule['build_frames'])
        if column in COST_COLUMNS and (unpriced or (rom_value == 1 and rule[column] == 0)):
            return POLICY
        return GAMEPLAY

    for profile, data in profiles.items():
        for rule in data['units']:
            raw = rule['raw']
            if not 0 <= raw < UNIT_COUNT:
                continue
            covered.add(raw)
            r = rom['units'][raw]
            for ecol, rcol in RULE_UNIT_COLUMNS:
                if rule[ecol] != r[rcol]:
                    add('units.rules', raw, ecol, r[rcol], rule[ecol], 'starcraft/race_rules.h',
                        profile, rule['enum'], rule_class(rule, ecol, r[rcol]))
            for key in ('ground', 'air'):
                attack = rule[key]
                if attack['stock']:
                    continue  # stock path: covered by the stock table diff
                if attack['damage'] <= 0:
                    # No attack of this class in the engine. Report a ROM
                    # weapon it drops; dedicated handlers are compared in
                    # weapons.engine instead.
                    if raw in DEDICATED_ATTACK and set(DEDICATED_ATTACK[raw][1]) & data.get('defines', set()):
                        continue
                    wid = r[key + '_weapon']
                    if raw in LAUNCHED_WEAPON and wid >= WEAPON_COUNT:
                        wid = rom['units'][LAUNCHED_WEAPON[raw]][key + '_weapon']
                    if wid < WEAPON_COUNT and rom['weapons'][wid]['damage'] and r[key + '_hits'] + (raw in LAUNCHED_WEAPON):
                        add('units.rules', raw, key + '_attack', f'weapon {wid}', 'none',
                            'starcraft/race_rules.h (attack eligibility)', profile, rule['enum'])
                    continue
                source_unit = r
                source = 'starcraft/race_rules.h (fallback weapon)'
                launched = raw in LAUNCHED_WEAPON and r[key + '_weapon'] >= WEAPON_COUNT
                if launched:
                    # Only the launched unit's damage is a weapon fact; the
                    # parent's launch cadence and reach are order/iscript data
                    # this layer does not read, so they stay uncovered.
                    source_unit = rom['units'][LAUNCHED_WEAPON[raw]]
                    source += f'; ROM weapon on launched unit {LAUNCHED_WEAPON[raw]}, damage only'
                wid = source_unit[key + '_weapon']
                if wid >= WEAPON_COUNT:
                    add('units.rules', raw, key + '_damage', 0, attack['damage'], source, profile, rule['enum'])
                    continue
                w = rom['weapons'][wid]
                columns = (('damage', w['damage']),) if launched else (
                    ('damage', w['damage']), ('cooldown', w['cooldown']),
                    ('range', w['max_range']), ('hits', source_unit[key + '_hits']))
                for column, rom_value in columns:
                    if attack[column] != rom_value:
                        add('units.rules', raw, f'{key}_{column}', rom_value, attack[column],
                            source, profile, rule['enum'])
        for rule in data['buildings']:
            raw = rule['raw']
            if not 0 <= raw < UNIT_COUNT:
                continue
            covered.add(raw)
            r = rom['units'][raw]
            for ecol, rcol in RULE_BUILDING_COLUMNS:
                if rule[ecol] != r[rcol]:
                    add('buildings.rules', raw, ecol, r[rcol], rule[ecol], 'starcraft/race_rules.h',
                        profile, rule['enum'], rule_class(rule, ecol, r[rcol]))
    diffs = []
    for (table, raw, column, rom_value, engine_value), info in sorted(merged.items(), key=lambda kv: (kv[0][0], kv[0][1], kv[0][2], str(kv[0][4]))):
        note = 'engine enum ' + ', '.join(sorted(info['enums']))
        extra = unit_note(rom['units'][raw], column)
        if extra:
            note += '; ' + extra
        diffs.append(cell(table, raw, unit_names.get(raw, f'unit {raw}'), column, rom_value,
                          engine_value, info['source'], info['profiles'], note, info['class']))
    return diffs, covered


def diff_research(rom: dict, engine: dict) -> tuple[list, list]:
    """Diff upgrades.dat and techdata.dat rows the engine prices; list the rest."""
    diffs, covered_up, covered_tech = [], set(), set()
    for upgrade, record in sorted(engine['upgrades'].items()):
        covered_up.add(upgrade)
        r = rom['upgrades'][upgrade]
        for column, value in record['values'].items():
            if value != r[column]:
                diffs.append(cell('upgrades', upgrade, UPGRADE_NAMES.get(upgrade, f'upgrade {upgrade}'),
                                  column, r[column], value, record['source'], record.get('profiles'),
                                  record.get('note')))
    for tech, record in sorted(engine['techdata'].items()):
        covered_tech.add(tech)
        r = rom['techdata'][tech]
        for column, value in record['values'].items():
            if value != r[column]:
                diffs.append(cell('techdata', tech, TECH_NAMES.get(tech, f'tech {tech}'),
                                  column, r[column], value, record['source'], note=record.get('note')))
    absent = []
    for i, r in enumerate(rom['upgrades']):
        if i not in covered_up and (r['mineral_base'] or r['gas_base'] or r['time_base']):
            absent.append({'provenance': PROVENANCE, 'table': 'upgrades', 'row': i,
                           'name': UPGRADE_NAMES.get(i, f'upgrade {i}'),
                           'rom': {k: r[k] for k in ('mineral_base', 'gas_base', 'time_base', 'max_level')},
                           'engine': 'absent'})
    for i, r in enumerate(rom['techdata']):
        if i not in covered_tech and (r['minerals'] or r['gas'] or r['research_time'] or r['energy']):
            absent.append({'provenance': PROVENANCE, 'table': 'techdata', 'row': i,
                           'name': TECH_NAMES.get(i, f'tech {i}'),
                           'rom': {k: r[k] for k in ('minerals', 'gas', 'research_time', 'energy')},
                           'engine': 'absent'})
    return diffs, absent


def diff_weapon_constants(rom: dict, constants: list, weapon_names: dict) -> tuple[list, list]:
    """Diff splash and spell constants; list ROM splash the engine lacks."""
    diffs, seen = [], set()
    for c in constants:
        if c['table'] != 'weapons':
            continue
        seen.add(c['row'])
        w = rom['weapons'][c['row']]
        rom_col = {'splash_inner': 'inner', 'splash_medium': 'medium',
                   'splash_outer': 'outer', 'spell_damage': 'damage'}.get(c['column'], c['column'])
        if w[rom_col] != c['value']:
            diffs.append(cell('weapons.engine', c['row'], weapon_names.get(c['row'], f'weapon {c["row"]}'),
                              c['column'], w[rom_col], c['value'], c['source']))
    absent = []
    for wid, w in enumerate(rom['weapons']):
        if wid not in seen and w['outer']:
            absent.append({'provenance': PROVENANCE, 'table': 'weapons.splash', 'row': wid,
                           'name': weapon_names.get(wid, f'weapon {wid}'),
                           'rom': {k: w[k] for k in ('inner', 'medium', 'outer')},
                           'engine': 'absent (no splash implementation)'})
    return diffs, absent


def absent_units(rom: dict, covered: set, unit_names: dict) -> list:
    """ROM units with a cost or HP that no release profile's race rules carry."""
    out = []
    for raw, r in enumerate(rom['units']):
        if raw in covered or not (r['hp'] or r['minerals']):
            continue
        out.append({'provenance': PROVENANCE, 'table': 'units.rules', 'row': raw,
                    'name': unit_names.get(raw, f'unit {raw}'),
                    'rom': {k: r[k] for k in ('hp', 'shields', 'minerals', 'gas', 'build_time')},
                    'engine': 'absent from release race rules'})
    return out


# --------------------------------------------------------------------------
# Engine side.

def stock_names(text: str, array: str) -> dict:
    body = text.split(array, 1)[1].split('};', 1)[0]
    return {int(m.group(1)): m.group(2).strip()
            for m in re.finditer(r'\[(\d+)\] = \{[^}]*\}, /\* ([^*]*)\*/', body)}


def read_constants(root: Path) -> list:
    out = []
    for table, row, column, header, pattern, group, scale in ENGINE_CONSTANTS:
        text = (root / header).read_text()
        m = re.search(pattern, text)
        if not m:
            raise ValueError(f'engine constant moved: {header} /{pattern}/')
        value = int(m.group(group))
        source = header
        if scale == 'ticks':
            source_frames = ticks_to_source_frames(value)
            if source_frames is None:
                raise ValueError(f'{header}: {value} ticks has no source-frame value')
            value, source = source_frames, f'{header} ({m.group(group)} ticks)'
        else:
            value *= scale
        out.append({'table': table, 'row': row, 'column': column, 'value': value, 'source': source})
    return out


def compile_and_run(root: Path, work: Path, name: str, flags: list) -> list[str]:
    binary = work / name
    subprocess.run(['clang', '-std=c11', '-O0', '-w', *flags,
                    str(root / 'tests/sc64_parity_tables_probe.c'), '-o', str(binary)],
                   cwd=root, check=True, capture_output=True, text=True)
    return subprocess.check_output([str(binary)], cwd=root, text=True).splitlines()


def parse_stock_lines(lines: list[str]) -> dict:
    out = {'stock_units': {}, 'stock_weapons': {}, 'sight_tiles': {}, 'armor_upgrade': {},
           'weapon_upgrade': {}, 'protoss_quotes': {}}
    for line in lines:
        f = line.split(',')
        kind, v = f[0], list(map(int, f[1:]))
        if kind == 'U':
            keys = ('flags', 'ground_weapon', 'air_weapon', 'ground_hits', 'air_hits', 'size',
                    'armor', 'minerals', 'gas', 'build_frames', 'supply_required',
                    'supply_provided', 'hp', 'shields', 'top_speed_q10')
            row = dict(zip(keys, v[1:]))
            row['building'] = row['flags'] & 1
            out['stock_units'][v[0]] = row
        elif kind == 'W':
            keys = ('damage', 'upgrade_bonus', 'damage_factor', 'cooldown', 'min_range',
                    'max_range', 'damage_class')
            out['stock_weapons'][v[0]] = dict(zip(keys, v[1:]))
        elif kind == 'S':
            out['sight_tiles'][v[0]] = v[1]
        elif kind == 'A':
            out['armor_upgrade'][v[0]] = v[1]
        elif kind == 'F':
            out['weapon_upgrade'][v[0]] = v[1]
        elif kind == 'Q':
            out['protoss_quotes'][(v[0], v[1])] = dict(zip(('mineral', 'gas', 'ticks', 'max_level'), v[2:]))
        else:
            raise ValueError('unrecognized probe row: ' + line)
    return out


def parse_rule_lines(lines: list[str]) -> dict:
    units, buildings = [], []
    for line in lines:
        f = line.split(',')
        if f[0] == 'R':
            v = list(map(int, f[2:]))
            keys = ('index', 'raw', 'minerals', 'gas', 'build_frames', 'supply',
                    'supply_provided', 'hp', 'shields', 'armor', 'flying')
            row = dict(zip(keys, v[:11]))
            for i, key in enumerate(('ground', 'air')):
                a = v[11 + 5 * i: 16 + 5 * i]
                row[key] = dict(zip(('stock', 'damage', 'cooldown', 'range', 'hits'), a))
            row['enum'] = f[1] or f'unit#{row["index"]}'
            units.append(row)
        elif f[0] == 'B':
            v = list(map(int, f[2:]))
            keys = ('index', 'raw', 'minerals', 'gas', 'build_frames', 'supply_provided',
                    'hp', 'shields', 'armor')
            row = dict(zip(keys, v))
            row['enum'] = f[1] or f'building#{row["index"]}'
            buildings.append(row)
        else:
            raise ValueError('unrecognized probe row: ' + line)
    return {'units': units, 'buildings': buildings}


def profile_flags(root: Path) -> dict:
    """Read the release profiles' -D/-I flags from the Makefile in one make run."""
    # $(info) prints through make itself, so flags holding shell quotes (the
    # audio blob define does) cannot break the command line.
    recipe = 'include Makefile\n.PHONY: parity-print-flags\nparity-print-flags:\n'
    for name, variables in PROFILES:
        joined = ' '.join(f'$({v})' for v in variables)
        recipe += f"\t@: $(info parity-flags:{name}={joined})\n"
    output = subprocess.run(['make', '--no-print-directory', '-s', '-f', '-', 'parity-print-flags'],
                            input=recipe, text=True, cwd=root, capture_output=True, check=True).stdout
    flags = {}
    for line in output.splitlines():
        if not line.startswith('parity-flags:'):
            continue
        name, _, value = line[len('parity-flags:'):].partition('=')
        if name in dict(PROFILES):
            # The compiler sees these through a shell: split with its quoting.
            # Keep defines and forced includes (the Carrier release selector
            # is one); include paths and code-generation flags do not matter.
            tokens, kept = shlex.split(value), []
            for i, token in enumerate(tokens):
                if token.startswith('-D'):
                    kept.append(token)
                elif token == '-include' and i + 1 < len(tokens):
                    kept += ['-include', tokens[i + 1]]  # repo-relative; probes run in root
            flags[name] = kept
    missing = [name for name, _ in PROFILES if not flags.get(name)]
    if missing:
        raise ValueError('empty profile flags: ' + ', '.join(missing))
    return flags


def profile_defines(root: Path, flags: list) -> set:
    """Every macro the profile defines, forced includes included."""
    out = subprocess.check_output(['clang', '-dM', '-E', '-x', 'c', *flags, '/dev/null'],
                                  cwd=root, text=True)
    return {line.split()[1].split('(')[0] for line in out.splitlines() if line.startswith('#define ')}


def enum_names_header(root: Path, flags: list, path: Path) -> None:
    prefix = (f'#include "{root}/common/gba.h"\n#include "{root}/starcraft/race_rules.h"\n')
    pre = subprocess.check_output(['clang', '-E', '-P', '-x', 'c', *flags, '-'], input=prefix,
                                  text=True, cwd=root, stderr=subprocess.DEVNULL)
    enums = re.findall(r'enum\s*\{(.*?)\}\s*;', pre, re.S)
    def names(marker, prefix_re):
        body = next(x for x in enums if marker in x)
        return list(dict.fromkeys(re.findall(r'\b(' + prefix_re + r'[A-Z0-9_]+)\b', body)))
    units = names('SC_RU_COUNT', 'U_')
    buildings = names('SC_RB_COUNT', 'B_')
    path.write_text(
        'static const char *parity_unit_names[SC_RU_COUNT] = {' +
        ','.join(f'[{n}]="{n}"' for n in units) + '};\n' +
        'static const char *parity_building_names[SC_RB_COUNT] = {' +
        ','.join(f'[{n}]="{n}"' for n in buildings) + '};\n')


def engine_research(stock: dict, root: Path, constants: list) -> dict:
    """Collect every upgrade and tech row the engine prices, in source frames."""
    upgrades, techdata = {}, {}
    quotes = stock['protoss_quotes']
    for family, upgrade in enumerate(PROTOSS_FAMILY_UPGRADES):
        q0, q1 = quotes.get((family, 0)), quotes.get((family, 1))
        if not q0:
            continue
        t0 = ticks_to_source_frames(q0['ticks'])
        values = {'mineral_base': q0['mineral'], 'gas_base': q0['gas'], 'time_base': t0,
                  'max_level': q0['max_level']}
        if q1:
            values.update({'mineral_factor': q1['mineral'] - q0['mineral'],
                           'gas_factor': q1['gas'] - q0['gas'],
                           'time_factor': ticks_to_source_frames(q1['ticks']) - t0})
        else:
            values.update({'mineral_factor': 0, 'gas_factor': 0, 'time_factor': 0})
        upgrades[upgrade] = {'values': values, 'source': 'starcraft/sc64_protoss_research.h',
                             'profiles': {'protoss-p10'}}
    text = (root / 'starcraft/game.c').read_text()
    found = {k: re.search(p, text) for k, p in GENERIC_RESEARCH_PATTERNS.items()}
    if not all(found.values()):
        raise ValueError('game.c start_research/tick_research moved: ' +
                         ', '.join(k for k, v in found.items() if not v))
    for race, lines in GENERIC_RESEARCH.items():
        for kind, ids in lines.items():
            ticks = int(found[kind + '_ticks'].group(1))
            generic = {'mineral_base': int(found['cost'].group(1)), 'mineral_factor': 0,
                       'gas_base': int(found['cost'].group(2)), 'gas_factor': 0,
                       'time_base': ticks_to_source_frames(ticks), 'time_factor': 0,
                       'max_level': int(found['max_level'].group(1))}
            for upgrade in ids:
                upgrades.setdefault(upgrade, {
                    'values': generic,
                    'source': f'starcraft/game.c start_research ({ticks} ticks, flat price)',
                    'note': f'{race} commander in every release ROM; one generic {kind} '
                            'level line raises every family of the race'})
    for c in constants:
        target = upgrades if c['table'] == 'upgrades' else techdata if c['table'] == 'techdata' else None
        if target is None:
            continue
        record = target.setdefault(c['row'], {'values': {}, 'source': c['source']})
        record['values'][c['column']] = c['value']
        if c['source'] not in record['source']:
            record['source'] += '; ' + c['source']
    for upgrade in (23, 24):
        # Colossus Reactor and Ventral Sacs are one-level, flat-price research.
        if upgrade in upgrades:
            upgrades[upgrade]['values'].update({'mineral_factor': 0, 'gas_factor': 0,
                                                'time_factor': 0, 'max_level': 1})
    return {'upgrades': upgrades, 'techdata': techdata}


def read_engine(root: Path, work: Path) -> dict:
    stock = parse_stock_lines(compile_and_run(root, work, 'stock', []))
    profiles = {}
    flags_by_profile = profile_flags(root)
    for name, _ in PROFILES:
        flags = flags_by_profile[name]
        names = work / f'{name}-names.h'
        enum_names_header(root, flags, names)
        # The probe reads the reference rule helpers. Z10's SC64_FACT_TABLES
        # swaps in generated tables built from those same helpers, and
        # tests/test_sc64_fact_tables.py checks the tables against them.
        lines = compile_and_run(root, work, name, ['-DPARITY_RULES', f'-DPARITY_NAMES="{names}"', *flags])
        profiles[name] = parse_rule_lines(lines)
        profiles[name]['defines'] = profile_defines(root, flags)
        profiles[name]['flags_sha256'] = hashlib.sha256(' '.join(flags).encode()).hexdigest()
    constants = read_constants(root)
    research = engine_research(stock, root, constants)
    return {'stock': stock, 'profiles': profiles, 'constants': constants, 'research': research}


# --------------------------------------------------------------------------
# Inputs, report and output.

def main_checkout(root: Path) -> Path:
    try:
        common = subprocess.check_output(['git', 'rev-parse', '--path-format=absolute', '--git-common-dir'],
                                         cwd=root, text=True, stderr=subprocess.DEVNULL).strip()
        return Path(common).parent
    except (subprocess.CalledProcessError, OSError):
        return root


def find_input(root: Path, relative: str, override: str | None) -> Path:
    if override:
        path = Path(override)
        if not path.is_file():
            raise InputsMissing(f'{path} does not exist')
        return path
    for base in (root, main_checkout(root)):
        if (base / relative).is_file():
            return base / relative
    raise InputsMissing(f'{relative} not found in this worktree or the main checkout')


def fixture_home(root: Path) -> Path:
    home = os.environ.get('SC64_PARITY_HOME')
    return Path(home) if home else main_checkout(root) / '.local/parity'


def check_private(root: Path, path: Path) -> None:
    """Refuse to write ROM-derived output where any git worktree would track it.

    The check runs on the resolved path, so a symlinked directory cannot lead
    into a tracked tree, and on whichever worktree contains it."""
    path = path.resolve()
    probe = path
    while not probe.exists():
        probe = probe.parent
    top = subprocess.run(['git', '-C', str(probe if probe.is_dir() else probe.parent),
                          'rev-parse', '--show-toplevel'], capture_output=True, text=True)
    bases = {root.resolve(), main_checkout(root).resolve()}
    if top.returncode == 0:
        bases.add(Path(top.stdout.strip()).resolve())
    for base in bases:
        if path.is_relative_to(base):
            ignored = subprocess.run(['git', 'check-ignore', '-q', str(path)], cwd=base).returncode == 0
            if not ignored:
                raise ValueError(f'{path} is not ignored by git; ROM-derived output must stay private')


def write_private(root: Path, outputs: dict) -> None:
    """Write {path: text} through fresh temporary files renamed into place.

    Every destination is validated before any is written, so a refusal leaves
    the earlier reports untouched. Files are created and renamed inside the
    resolved directory; os.replace swaps the directory entry, so a hard link at
    the destination is replaced rather than written through."""
    planned = []
    for path, text in outputs.items():
        parent = path.parent.resolve()
        entry = parent / path.name
        if entry.is_symlink():
            raise ValueError(f'{entry} is a symlink; refusing to replace it')
        if entry.exists() and not entry.is_file():
            raise ValueError(f'{entry} exists and is not a regular file')
        check_private(root, parent)
        # The unresolved entry is what os.replace overwrites; git check-ignore
        # never reports a tracked entry as ignored, so a tracked one is refused.
        check_private(root, entry)
        planned.append((parent, entry, text))
    temps = []
    try:
        for parent, entry, text in planned:
            with tempfile.NamedTemporaryFile('w', dir=parent, prefix='.' + entry.name + '.',
                                             delete=False) as f:
                temps.append(Path(f.name))
                f.write(text)
        for temp, (_, entry, _) in zip(temps, planned):
            os.replace(temp, entry)
    finally:
        for temp in temps:
            temp.unlink(missing_ok=True)


def report_lines(result: dict) -> list[str]:
    lines = []
    def fmt(v):
        return json.dumps(v) if isinstance(v, (dict, list)) else str(v)
    order = ('units.stock', 'units.rules', 'buildings.rules', 'weapons.stock', 'weapons.engine',
             'upgrades', 'techdata')
    for table in order:
        rows = [d for d in result['diffs'] if d['table'] == table]
        policy = sum(d['class'] == POLICY for d in rows)
        lines.append(f'== {table}: {len(rows)} differing cells' +
                     (f' ({policy} policy: authored zero cost or ROM placeholder)' if policy else ''))
        for d in rows:
            extra = []
            if d.get('profiles'):
                extra.append('profiles ' + ','.join(d['profiles']))
            if d['class'] != GAMEPLAY:
                extra.append(d['class'])
            if d.get('note'):
                extra.append(d['note'])
            lines.append(f"{d['table']}  {d['row']:>3} {d['name']:<38} {d['column']:<16} "
                         f"ROM={fmt(d['rom']):<8} engine={fmt(d['engine']):<8} "
                         f"[{d['engine_source']}]" + (' (' + '; '.join(extra) + ')' if extra else ''))
    lines.append(f"== absent: {len(result['absent'])} ROM rows with no engine counterpart")
    for a in result['absent']:
        lines.append(f"{a['table']}  {a['row']:>3} {a['name']:<38} ROM={fmt(a['rom'])}  engine={a['engine']}")
    lines.append(f"== speed rounding (|delta| <= {SPEED_ROUNDING_Q10} Q10, not counted): {len(result['rounding'])}")
    for d in result['rounding']:
        lines.append(f"units.stock  {d['row']:>3} {d['name']:<38} top_speed_q10 ROM={d['rom']} engine={d['engine']}")
    lines.append(f"== speed not comparable from flingy.dat (iscript mover or engine-immobile, not counted): {len(result['iscript_speed'])}")
    for d in result['iscript_speed']:
        lines.append(f"units.stock  {d['row']:>3} {d['name']:<38} top_speed_q10 flingy={d['rom']} engine={d['engine']} ({d['note']})")
    lines.append('== not covered: per-mission CHK opening censuses and iscript sequences (not implemented)')
    lines.append('== summary: ' + ', '.join(f'{k} {v}' for k, v in result['counts'].items()))
    return lines


def build_result(rom: dict, engine: dict, unit_names: dict, weapon_names: dict) -> dict:
    stock_diffs, rounding, iscript_speed = diff_stock(rom, engine['stock'], unit_names, weapon_names)
    rule_diffs, covered = diff_rules(rom, engine['profiles'], unit_names)
    research_diffs, research_absent = diff_research(rom, engine['research'])
    weapon_diffs, splash_absent = diff_weapon_constants(rom, engine['constants'], weapon_names)
    diffs = stock_diffs + rule_diffs + weapon_diffs + research_diffs
    absent = absent_units(rom, covered, unit_names) + splash_absent + research_absent
    counts = {}
    for d in diffs:
        counts[d['table']] = counts.get(d['table'], 0) + 1
        if d['class'] == POLICY:
            counts[d['table'] + ' (policy)'] = counts.get(d['table'] + ' (policy)', 0) + 1
    counts['total'] = len(diffs)
    counts['total gameplay'] = sum(d['class'] == GAMEPLAY for d in diffs)
    counts['absent'] = len(absent)
    return {'diffs': diffs, 'absent': absent, 'rounding': rounding,
            'iscript_speed': iscript_speed, 'counts': counts}


def run(args) -> int:
    root = Path(args.root).resolve()
    try:
        rom_path = find_input(root, '.local/sc64/rom/StarCraft64-USA.z64', args.rom)
        extractor = find_input(root, 'references/sc64-maps/extract_sc64_maps.py', args.extractor)
    except InputsMissing as missing:
        print(f'SKIP sc64_parity_tables: private ROM inputs absent ({missing}). '
              'Pass --rom and --extractor, or prepare .local/sc64/rom.', file=sys.stderr)
        return 2 if args.require_inputs else 0
    from tools.sc64_openbw_oracle_tables import EXPECTED_ROM_SHA1, TABLES, read_tables
    tables = read_tables(rom_path, extractor)
    rom = parse_rom_tables(tables)
    profiles_text = (root / 'starcraft/sc64_stock_profiles.h').read_text()
    unit_names = stock_names(profiles_text, 'sc64_stock_unit_profiles[')
    weapon_names = stock_names(profiles_text, 'sc64_stock_weapon_profiles[')
    with tempfile.TemporaryDirectory(prefix='sc64-parity-tables-') as temp:
        engine = read_engine(root, Path(temp))
    result = build_result(rom, engine, unit_names, weapon_names)
    lines = report_lines(result)
    print('\n'.join(lines))
    # Resolve once and write only through the resolved directory, so a link
    # anywhere in the given path cannot redirect the reports after the check.
    out_dir = (Path(args.out) if args.out else fixture_home(root) / 'tables').resolve()
    check_private(root, out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    check_private(root, out_dir)
    commit = subprocess.run(['git', 'rev-parse', 'HEAD'], cwd=root, text=True,
                            capture_output=True).stdout.strip()
    document = {
        'schema': SCHEMA, 'provenance': PROVENANCE, 'tool': 'tools/sc64_parity_tables.py',
        'engine_commit': commit, 'rom_sha1': EXPECTED_ROM_SHA1,
        'table_sha256': {name: digest for name, (_, digest) in TABLES.items()},
        'profiles': {name: {'variables': list(variables), 'flags_sha256': engine['profiles'][name]['flags_sha256']}
                     for name, variables in PROFILES},
        'normalisation': ['turret weapons from subunit rows', 'hp 8.8 fixed point (integer part)',
                          'shields gated by shield-enable', 'speed = flingy top speed * 4 (Q10)',
                          'times in source frames; engine ticks inverted from (x*5+3)/4'],
        'not_covered': ['per-mission CHK opening censuses', 'iscript animation and sound sequences'],
        **result,
    }
    for name in engine['profiles']:
        engine['profiles'][name].pop('defines', None)
    write_private(root, {out_dir / 'rom-table-diff.json': json.dumps(document, indent=1) + '\n',
                         out_dir / 'rom-table-diff.txt': '\n'.join(lines) + '\n'})
    print(f'wrote {out_dir / "rom-table-diff.json"}', file=sys.stderr)
    return 1 if args.fail_on_diff and result['diffs'] else 0


def main(argv=None) -> int:
    p = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    p.add_argument('--root', default=str(ROOT), help='engine source tree (default: this checkout)')
    p.add_argument('--rom', help='StarCraft 64 USA ROM (default: .local/sc64/rom/StarCraft64-USA.z64)')
    p.add_argument('--extractor', help='pinned BOLT extractor (default: references/sc64-maps/extract_sc64_maps.py)')
    p.add_argument('--out', help='output directory (default: $SC64_PARITY_HOME/tables)')
    p.add_argument('--require-inputs', action='store_true', help='exit 2 instead of skipping when inputs are absent')
    p.add_argument('--fail-on-diff', action='store_true', help='exit 1 when any cell differs')
    return run(p.parse_args(argv))


if __name__ == '__main__':
    sys.exit(main())
