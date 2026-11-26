#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Checks for the offline ROM-table differ (tools/sc64_parity_tables.py).

The normalisation and diff tests use small synthetic tables and need no game
data. The last test runs the real differ only when the private ROM inputs are
present, and skips with a message otherwise.
"""
import json
import re
import struct
import subprocess
import tempfile
import unittest
from pathlib import Path

from tools import sc64_parity_tables as pt
from tools.sc64_weapon_source_audit import FLINGY_FIELDS, WEAPON_FIELDS

ROOT = Path(__file__).resolve().parents[1]


def pack(fields, count, values):
    """Build a column-major table; values maps field -> {row: value}."""
    out = bytearray()
    for spec in fields:
        name, fmt = spec[0], spec[1]
        first, rows = (spec[2], spec[3]) if len(spec) > 2 else (0, count)
        column = values.get(name, {})
        for i in range(first, first + rows):
            out += struct.pack('<' + fmt, column.get(i, 0))
    return bytes(out)


def synthetic_tables(units=None, weapons=None, flingy=None, upgrades=None, techdata=None):
    units = {'ground_weapon': {i: pt.NONE_WEAPON for i in range(pt.UNIT_COUNT)},
             'air_weapon': {i: pt.NONE_WEAPON for i in range(pt.UNIT_COUNT)},
             'subunit1': {i: pt.NONE_UNIT for i in range(pt.UNIT_COUNT)},
             **(units or {})}
    return {
        'units.dat': pack(pt.UNIT_FIELDS, pt.UNIT_COUNT, units),
        'weapons.dat': pack(WEAPON_FIELDS, pt.WEAPON_COUNT, weapons or {}),
        'flingy.dat': pack(FLINGY_FIELDS, pt.FLINGY_COUNT, flingy or {}),
        'upgrades.dat': pack(pt.UPGRADE_FIELDS, pt.UPGRADE_COUNT, upgrades or {}),
        'techdata.dat': pack(pt.TECH_FIELDS, pt.TECH_COUNT, techdata or {}),
    }


def stock_engine(rom):
    """An engine stock table equal to the normalised ROM rows."""
    units = {}
    for raw, r in enumerate(rom['units']):
        units[raw] = {'building': r['building'], 'ground_weapon': r['ground_weapon'],
                      'air_weapon': r['air_weapon'], 'ground_hits': r['ground_hits'],
                      'air_hits': r['air_hits'], 'size': r['size'], 'armor': r['armor'],
                      'minerals': r['minerals'], 'gas': r['gas'], 'build_frames': r['build_time'],
                      'supply_required': r['supply_required'],
                      'supply_provided': r['supply_provided'], 'hp': r['hp'],
                      'shields': r['shields'], 'top_speed_q10': r['top_speed_q10']}
    weapons = {i: {'damage': w['damage'], 'upgrade_bonus': w['bonus'], 'damage_factor': w['count'],
                   'cooldown': w['cooldown'], 'min_range': w['min_range'],
                   'max_range': w['max_range'], 'damage_class': w['damage_type']}
               for i, w in enumerate(rom['weapons'])}
    return {'stock_units': units, 'stock_weapons': weapons, 'sight_tiles': {},
            'armor_upgrade': {}, 'weapon_upgrade': {}}


def rule(raw, enum, **values):
    row = {'raw': raw, 'enum': enum, 'minerals': 0, 'gas': 0, 'build_frames': 0, 'supply': 0,
           'supply_provided': 0, 'hp': 0, 'shields': 0, 'armor': 0, 'flying': 0,
           'ground': {'stock': 1, 'damage': 0, 'cooldown': 0, 'range': 0, 'hits': 0},
           'air': {'stock': 0, 'damage': 0, 'cooldown': 0, 'range': 0, 'hits': 0}}
    row.update(values)
    return row


class LayoutTest(unittest.TestCase):
    # PC units.dat byte offsets recorded independently of this parser in the
    # repository's audits (docs/audits/2026-11-19-world-depth.md,
    # tools/sc64_audio_pack.py, docs/P8-HYPERION-FACTS.md) and the C10 audit.
    UNIT_OFFSETS = {'shields': 2700, 'hp': 3156, 'elevation': 4068,
                    'ground_weapon': 5892, 'air_weapon': 6348, 'sight': 8172,
                    'armor_upgrade': 8400, 'size': 8628, 'armor': 8856,
                    'ready_sound': 9312, 'what_start': 9524, 'what_end': 9980,
                    'yes_start': 10860, 'yes_end': 11072, 'minerals': 14860,
                    'gas': 15316, 'build_time': 15772, 'supply_provided': 16912,
                    'supply_required': 17140}

    def test_unit_columns_sit_at_the_recorded_offsets(self):
        offsets, cursor = {}, 0
        for name, fmt, _, rows in pt.UNIT_FIELDS:
            offsets[name] = cursor
            cursor += rows * struct.calcsize('<' + fmt)
        self.assertEqual(cursor, pt.SIZES['units.dat'])
        for name, offset in self.UNIT_OFFSETS.items():
            self.assertEqual(offsets[name], offset, name)

    def test_distinct_values_read_back_from_fixed_offsets(self):
        tables = synthetic_tables()
        units = bytearray(tables['units.dat'])
        struct.pack_into('<I', units, 3156 + 4 * 65, 80 << 8)
        struct.pack_into('<H', units, 2700 + 2 * 65, 81)
        struct.pack_into('<H', units, 14860 + 2 * 65, 82)
        struct.pack_into('<H', units, 15772 + 2 * 65, 83)
        struct.pack_into('<B', units, 17140 + 65, 84)
        tables['units.dat'] = bytes(units)
        z = pt.parse_rom_tables(tables)['units'][65]
        self.assertEqual((z['hp'], z['shields_raw'], z['minerals'], z['build_time'],
                          z['supply_required']), (80, 81, 82, 83, 84))
        self.assertEqual(pt.SIZES['upgrades.dat'], 61 * 21)
        self.assertEqual(pt.SIZES['techdata.dat'], 44 * 19)


class NormalisationTest(unittest.TestCase):
    def setUp(self):
        self.rom = pt.parse_rom_tables(synthetic_tables(
            units={'subunit1': {i: (4 if i == 3 else pt.NONE_UNIT) for i in range(pt.UNIT_COUNT)},
                   'ground_weapon': {i: (7 if i == 4 else 0 if i == 0 else pt.NONE_WEAPON)
                                     for i in range(pt.UNIT_COUNT)},
                   'air_weapon': {i: (8 if i == 4 else pt.NONE_WEAPON) for i in range(pt.UNIT_COUNT)},
                   'ground_hits': {4: 1}, 'air_hits': {4: 2},
                   'hp': {0: 40 << 8, 65: (80 << 8) | 128},
                   'shield_enable': {65: 1}, 'shields': {65: 80, 0: 100},
                   'flingy': {0: 2, 65: 3}, 'minerals': {65: 100}, 'build_time': {65: 600}},
            flingy={'top_speed': {2: 1024, 3: 1000}, 'movement_control': {2: 2}}))

    def test_turret_weapons_come_from_the_subunit_row(self):
        tank = self.rom['units'][3]
        self.assertEqual((tank['ground_weapon'], tank['air_weapon'], tank['air_hits']), (7, 8, 2))
        self.assertIn('weapons read from turret subunit 4', tank['notes'])
        self.assertEqual(self.rom['units'][5]['ground_weapon'], pt.NONE_WEAPON)

    def test_hp_is_8_8_fixed_point(self):
        self.assertEqual(self.rom['units'][0]['hp'], 40)
        zealot = self.rom['units'][65]
        self.assertEqual((zealot['hp'], zealot['hp_raw']), (80, 20608))
        self.assertTrue(any('fraction 128/256' in n for n in zealot['notes']))

    def test_shields_need_the_enable_column(self):
        self.assertEqual(self.rom['units'][65]['shields'], 80)
        self.assertEqual(self.rom['units'][0]['shields'], 0)
        self.assertTrue(any('disabled' in n for n in self.rom['units'][0]['notes']))

    def test_speed_is_flingy_top_speed_in_q10(self):
        self.assertEqual(self.rom['units'][65]['top_speed_q10'], 4000)
        self.assertEqual(self.rom['units'][0]['movement_control'], 2)

    def test_engine_ticks_invert_to_source_frames(self):
        for source in (0, 1, 2, 480, 1800, 2400, 4000, 4480):
            self.assertEqual(pt.ticks_to_source_frames((source * 5 + 3) // 4), source)
        self.assertIsNone(pt.ticks_to_source_frames(1))
        self.assertIsNone(pt.ticks_to_source_frames(6))

    def test_short_tables_are_refused(self):
        tables = synthetic_tables()
        tables['units.dat'] = tables['units.dat'][:-1]
        with self.assertRaises(ValueError):
            pt.parse_rom_tables(tables)


class DiffTest(unittest.TestCase):
    def setUp(self):
        self.rom = pt.parse_rom_tables(synthetic_tables(
            units={'hp': {65: 80 << 8, 12: 500 << 8, 13: 20 << 8},
                   'shield_enable': {65: 1}, 'shields': {65: 80},
                   'minerals': {65: 100, 12: 400, 13: 1}, 'build_time': {65: 600, 12: 2400, 13: 1},
                   'supply_required': {12: 16}, 'flingy': {65: 1, 12: 2, 0: 3, 7: 4, 110: 4},
                   'special_flags': {110: 1},
                   'ground_weapon': {i: (84 if i == 67 else pt.NONE_WEAPON) for i in range(pt.UNIT_COUNT)},
                   'subunit1': {i: pt.NONE_UNIT for i in range(pt.UNIT_COUNT)}},
            weapons={'damage': {84: 16}, 'outer': {84: 48, 25: 25}, 'inner': {84: 48},
                     'medium': {84: 48}, 'cooldown': {84: 45}},
            flingy={'top_speed': {1: 1000, 2: 640, 3: 1, 4: 500}, 'movement_control': {3: 2}},
            upgrades={'mineral_base': {5: 100, 16: 150}, 'mineral_factor': {5: 75},
                      'gas_base': {5: 100}, 'gas_factor': {5: 75}, 'time_base': {5: 4000},
                      'time_factor': {5: 480}, 'max_level': {5: 3}},
            techdata={'energy': {19: 75, 1: 100}}))
        self.names = {65: 'Protoss_Zealot', 12: 'Terran_Battlecruiser'}

    def test_stock_cells_rounding_and_iscript_speed(self):
        engine = stock_engine(self.rom)
        engine['stock_units'][65].update(hp=100, shields=60, top_speed_q10=4002)
        engine['stock_units'][12].update(build_frames=2000, top_speed_q10=1)
        engine['stock_units'][0].update(top_speed_q10=4096)
        engine['stock_units'][7].update(top_speed_q10=0)
        engine['stock_units'][110].update(top_speed_q10=0)  # building that never moves
        engine['stock_weapons'][84].update(damage=14)
        del engine['stock_units'][200]
        diffs, rounding, iscript = pt.diff_stock(self.rom, engine, self.names, {84: 'Psionic_Storm'})
        cells = {(d['row'], d['column']): (d['rom'], d['engine']) for d in diffs}
        self.assertEqual(cells[(65, 'hp')], (80, 100))
        self.assertEqual(cells[(65, 'shields')], (80, 60))
        self.assertEqual(cells[(12, 'build_frames')], (2400, 2000))
        self.assertEqual(cells[(12, 'top_speed_q10')], (2560, 1))
        self.assertEqual(cells[(84, 'damage')], (16, 14))
        self.assertEqual(cells[(200, 'row')], ('present', 'absent'))
        self.assertEqual([(d['row'], d['rom']) for d in rounding], [(65, 4000)])
        self.assertEqual(cells[(7, 'top_speed_q10')], (2000, 0))  # a stopped mover is reported
        self.assertEqual(sorted(d['row'] for d in iscript), [0, 110])
        self.assertTrue(all(d['provenance'] == 'rom-table' for d in diffs + rounding + iscript))

    def test_rules_merge_profiles_and_classify_policy(self):
        profiles = {
            'a': {'units': [rule(65, 'U_ZEALOT', minerals=100, build_frames=600, hp=100, shields=60),
                            rule(13, 'U_SPIDER_MINE', hp=20)],
                  'buildings': []},
            'b': {'units': [rule(65, 'U_ZEALOT', minerals=100, build_frames=600, hp=100, shields=60),
                            rule(12, 'U_BC', minerals=400, build_frames=2000, hp=500, supply=12)],
                  'buildings': []},
        }
        diffs, covered = pt.diff_rules(self.rom, profiles, self.names)
        by = {(d['row'], d['column']): d for d in diffs}
        self.assertEqual(by[(65, 'hp')]['profiles'], ['a', 'b'])
        self.assertEqual(by[(65, 'hp')]['class'], pt.GAMEPLAY)
        self.assertEqual(by[(12, 'supply')]['profiles'], ['b'])
        self.assertEqual(by[(13, 'minerals')]['class'], pt.POLICY)
        self.assertEqual(covered, {12, 13, 65})
        absent = pt.absent_units(self.rom, covered, self.names)
        self.assertEqual([a['row'] for a in absent], [])

    def test_fallback_weapon_is_compared_against_the_rom_weapon(self):
        profiles = {'a': {'units': [rule(67, 'U_HT', ground={'stock': 0, 'damage': 14, 'cooldown': 45,
                                                              'range': 288, 'hits': 0})],
                          'buildings': []}}
        diffs, _ = pt.diff_rules(self.rom, profiles, {})
        cells = {d['column']: (d['rom'], d['engine']) for d in diffs}
        self.assertEqual(cells['ground_damage'], (16, 14))
        self.assertEqual(cells['ground_range'], (0, 288))
        self.assertNotIn('ground_cooldown', cells)

    def test_dedicated_handler_exemption_needs_the_handler(self):
        rom = pt.parse_rom_tables(synthetic_tables(
            units={'ground_weapon': {i: (54 if i == 50 else pt.NONE_WEAPON) for i in range(pt.UNIT_COUNT)},
                   'ground_hits': {50: 1},
                   'subunit1': {i: pt.NONE_UNIT for i in range(pt.UNIT_COUNT)}},
            weapons={'damage': {54: 500}}))
        unarmed = rule(50, 'U_INFESTED', ground={'stock': 0, 'damage': 0, 'cooldown': 0, 'range': 0, 'hits': 0})
        diffs, _ = pt.diff_rules(rom, {
            'with': {'units': [unarmed], 'buildings': [], 'defines': {'SC64_P6_COMBAT_ENABLED'}},
            'without': {'units': [unarmed], 'buildings': [], 'defines': set()}}, {})
        self.assertEqual([(d['column'], d['profiles']) for d in diffs], [('ground_attack', ['without'])])

    def test_launched_weapon_compares_damage_only(self):
        rom = pt.parse_rom_tables(synthetic_tables(
            units={'ground_weapon': {i: (82 if i == 85 else pt.NONE_WEAPON) for i in range(pt.UNIT_COUNT)},
                   'subunit1': {i: pt.NONE_UNIT for i in range(pt.UNIT_COUNT)}},
            weapons={'damage': {82: 100}, 'cooldown': {82: 1}, 'max_range': {82: 128}}))
        reaver = rule(83, 'U_REAVER', ground={'stock': 0, 'damage': 90, 'cooldown': 60,
                                              'range': 256, 'hits': 1})
        diffs, _ = pt.diff_rules(rom, {'a': {'units': [reaver], 'buildings': []}}, {})
        self.assertEqual([(d['column'], d['rom'], d['engine']) for d in diffs],
                         [('ground_damage', 100, 90)])

    def test_research_and_constants(self):
        engine = {'upgrades': {5: {'values': {'mineral_base': 100, 'mineral_factor': 75, 'gas_base': 100,
                                              'gas_factor': 75, 'time_base': 4000, 'time_factor': 480,
                                              'max_level': 3}, 'source': 'x.h'}},
                  'techdata': {19: {'values': {'energy': 75}, 'source': 'storm.h'}}}
        diffs, absent = pt.diff_research(self.rom, engine)
        self.assertEqual(diffs, [])
        self.assertEqual(sorted((a['table'], a['row']) for a in absent),
                         [('techdata', 1), ('upgrades', 16)])
        constants = [{'table': 'weapons', 'row': 84, 'column': 'splash_outer', 'value': 48, 'source': 's'},
                     {'table': 'weapons', 'row': 84, 'column': 'spell_damage', 'value': 14, 'source': 's'}]
        diffs, absent = pt.diff_weapon_constants(self.rom, constants, {})
        self.assertEqual([(d['column'], d['rom'], d['engine']) for d in diffs], [('spell_damage', 16, 14)])
        self.assertEqual([a['row'] for a in absent], [25])

    def test_report_names_every_cell_and_section(self):
        engine = stock_engine(self.rom)
        engine['stock_units'][65].update(hp=100)
        result = pt.build_result(self.rom, {'stock': engine, 'profiles': {}, 'constants': [],
                                            'research': {'upgrades': {}, 'techdata': {}}},
                                 self.names, {})
        text = '\n'.join(pt.report_lines(result))
        self.assertIn('Protoss_Zealot', text)
        self.assertIn('ROM=80', text)
        self.assertIn('engine=100', text)
        self.assertIn('not covered', text)
        self.assertEqual(result['counts']['units.stock'], 1)


class EngineSourceTest(unittest.TestCase):
    def test_engine_constants_are_found(self):
        constants = pt.read_constants(ROOT)
        self.assertEqual(len(constants), len(pt.ENGINE_CONSTANTS))
        storm = {c['column']: c['value'] for c in constants if c['table'] == 'weapons' and c['row'] == 84}
        self.assertEqual(storm['splash_outer'], 48)
        sacs = [c for c in constants if c['table'] == 'upgrades' and c['row'] == 24 and c['column'] == 'time_base']
        self.assertEqual(sacs[0]['value'], 2400)  # 3000 engine ticks

    def test_stock_probe_covers_every_raw_row(self):
        with tempfile.TemporaryDirectory() as temp:
            stock = pt.parse_stock_lines(pt.compile_and_run(ROOT, Path(temp), 'stock', []))
        self.assertEqual(len(stock['stock_units']), pt.UNIT_COUNT)
        self.assertEqual(len(stock['stock_weapons']), 132)
        self.assertEqual(len(stock['sight_tiles']), pt.UNIT_COUNT)
        self.assertEqual(sorted({family for family, _ in stock['protoss_quotes']}), list(range(6)))

    def test_output_must_stay_private(self):
        with self.assertRaises(ValueError):
            pt.check_private(ROOT, ROOT / 'tools' / 'parity-output')
        pt.check_private(ROOT, ROOT / '.local' / 'parity' / 'tables')

    def test_writes_never_follow_links_into_tracked_files(self):
        with tempfile.TemporaryDirectory() as temp:
            repo = Path(temp)
            subprocess.run(['git', 'init', '-q', str(repo)], check=True)
            (repo / '.gitignore').write_text('private/\n')
            (repo / 'tracked.c').write_text('keep\n')
            (repo / 'private').mkdir()
            (repo / 'private' / 'rom-table-diff.json').symlink_to(repo / 'tracked.c')
            (repo / 'private' / 'other.json').write_text('x\n')
            (repo / 'private' / 'link.json').symlink_to(repo / 'private' / 'other.json')
            with self.assertRaises(ValueError):  # any symlink destination: refused
                pt.write_private(repo, {repo / 'private' / 'link.json': 'derived\n'})
            self.assertTrue((repo / 'private' / 'link.json').is_symlink())
            with self.assertRaises(ValueError):  # link into a tracked file: refused
                pt.write_private(repo, {repo / 'private' / 'rom-table-diff.json': 'derived\n'})
            self.assertEqual((repo / 'tracked.c').read_text(), 'keep\n')
            (repo / 'private' / 'rom-table-diff.json').unlink()
            (repo / 'private' / 'rom-table-diff.json').hardlink_to(repo / 'tracked.c')
            pt.write_private(repo, {repo / 'private' / 'rom-table-diff.json': 'derived\n'})
            self.assertEqual((repo / 'tracked.c').read_text(), 'keep\n')  # entry replaced
            self.assertEqual((repo / 'private' / 'rom-table-diff.json').read_text(), 'derived\n')
            (repo / 'linked').symlink_to(repo / 'src', target_is_directory=True)
            (repo / 'src').mkdir()
            with self.assertRaises(ValueError):
                pt.write_private(repo, {repo / 'linked' / 'rom-table-diff.json': 'derived\n'})
            with self.assertRaises(ValueError):
                pt.check_private(ROOT, repo / 'src' / 'out')
            # A refused second destination leaves the first report unwritten.
            (repo / 'private' / 'a.json').write_text('old\n')
            with self.assertRaises(ValueError):
                pt.write_private(repo, {repo / 'private' / 'a.json': 'new\n',
                                        repo / 'private' / 'link.json': 'new\n'})
            self.assertEqual((repo / 'private' / 'a.json').read_text(), 'old\n')
            (repo / 'private' / 'dir.txt').mkdir()
            with self.assertRaises(ValueError):
                pt.write_private(repo, {repo / 'private' / 'a.json': 'new\n',
                                        repo / 'private' / 'dir.txt': 'new\n'})
            self.assertEqual((repo / 'private' / 'a.json').read_text(), 'old\n')
            self.assertEqual(sorted(x.name for x in (repo / 'private').iterdir() if x.name.startswith('.')), [])

    def test_missing_inputs_skip_cleanly(self):
        with tempfile.TemporaryDirectory() as temp:
            missing = str(Path(temp) / 'absent.z64')
            code = pt.main(['--rom', missing, '--extractor', missing])
            self.assertEqual(code, 0)
            self.assertEqual(pt.main(['--rom', missing, '--extractor', missing, '--require-inputs']), 2)


class RomTablesTest(unittest.TestCase):
    """Runs only where the private ROM and extractor are prepared."""

    def test_known_rom_differences_are_reported(self):
        try:
            pt.find_input(ROOT, '.local/sc64/rom/StarCraft64-USA.z64', None)
            pt.find_input(ROOT, 'references/sc64-maps/extract_sc64_maps.py', None)
        except pt.InputsMissing as missing:
            self.skipTest(f'private ROM inputs absent: {missing}')
        with tempfile.TemporaryDirectory() as temp:
            out = Path(temp) / 'tables'
            run = subprocess.run(['python3', 'tools/sc64_parity_tables.py', '--out', str(out)],
                                 cwd=ROOT, capture_output=True, text=True)
            self.assertEqual(run.returncode, 0, run.stderr)
            document = json.loads((out / 'rom-table-diff.json').read_text())
        # The ROM is pinned, so its side is fixed; the engine side may change.
        # A cell must be reported exactly when the engine value differs.
        from tools.sc64_openbw_oracle_tables import read_tables
        rom = pt.parse_rom_tables(read_tables(
            pt.find_input(ROOT, '.local/sc64/rom/StarCraft64-USA.z64', None),
            pt.find_input(ROOT, 'references/sc64-maps/extract_sc64_maps.py', None)))
        expected = {(65, 'hp'): 80, (65, 'shields'): 80, (12, 'build_time'): 2400,
                    (70, 'minerals'): 300, (66, 'build_time'): 600}
        for (raw, column), value in expected.items():
            self.assertEqual(rom['units'][raw][column], value, (raw, column))
        self.assertEqual(rom['weapons'][84]['damage'], 16)
        # Engine values read straight from the rule-table text, independently
        # of the probe: race, flags, producer, supply, minerals, gas,
        # build_frames, hp, shields.
        rules = (ROOT / 'starcraft/race_rules.h').read_text()
        def rule_row(enum):
            body = re.search(r'\[' + enum + r'\]\s*=\s*\{([^}]*)\}', rules).group(1)
            fields = [x.strip() for x in body.split(',')]
            return dict(zip(('minerals', 'gas', 'build_frames', 'hp', 'shields'),
                            map(int, fields[4:9])))
        engine = {65: rule_row('U_ZEALOT'), 66: rule_row('U_DRAGOON'), 70: rule_row('U_SCOUT'),
                  12: rule_row('U_BATTLECRUISER')}
        rule_rows = {(d['row'], d['column']): d for d in document['diffs'] if d['table'] == 'units.rules'}
        for (raw, column), value in expected.items():
            engine_column = 'build_frames' if column == 'build_time' else column
            if raw not in engine:
                continue
            reported = rule_rows.get((raw, engine_column))
            if engine[raw][engine_column] == value:
                self.assertIsNone(reported, (raw, column))
            else:
                self.assertIsNotNone(reported, (raw, column))
                self.assertEqual((reported['rom'], reported['engine']),
                                 (value, engine[raw][engine_column]))
        stock = (ROOT / 'starcraft/sc64_stock_profiles.h').read_text()
        weapons = stock[stock.index('sc64_stock_weapon_profiles['):]
        storm_engine = int(re.search(r'\[84\] = \{ (\d+),', weapons).group(1))
        storm = [d for d in document['diffs'] if d['table'] == 'weapons.stock' and d['row'] == 84
                 and d['column'] == 'damage']
        self.assertEqual(bool(storm), storm_engine != 16)
        self.assertTrue(all(d['provenance'] == 'rom-table' for d in document['diffs'] + document['absent']))


if __name__ == '__main__':
    unittest.main()
