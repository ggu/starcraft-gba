#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Regression checks for the stock-profile audit's current helper semantics."""
import tempfile
import json
import unittest
from pathlib import Path

from tools.sc64_unit_spec_engine import dump
from tools.sc64_unit_spec_compare import compare, row_for


ROOT = Path(__file__).resolve().parents[1]


def unit_fact(raw, name, top_speed, ground_weapon, air_weapon,
              ground_hits=1, air_hits=1, hp=100, shields=0, armor=0,
              minerals=0, gas=0, build_frames=0, supply=0, supply_provided=0):
    return {
        'id': raw, 'name': name, 'top_speed': top_speed,
        'ground_weapon': {'id': ground_weapon}, 'air_weapon': {'id': air_weapon},
        'ground_weapon_hits': ground_hits, 'air_weapon_hits': air_hits,
        'is_building': False, 'hp': hp, 'shields': shields, 'armor': armor,
        'mineral_cost': minerals, 'gas_cost': gas, 'build_time': build_frames,
        'supply_required': supply, 'supply_provided': supply_provided,
        'size': {'id': 3},
    }


def weapon_fact(weapon_id, name, damage, bonus, factor, cooldown,
                minimum, maximum, damage_class):
    return {
        'id': weapon_id, 'name': name, 'damage_amount': damage,
        'damage_bonus': bonus, 'damage_factor': factor,
        'damage_cooldown': cooldown, 'min_range': minimum,
        'max_range': maximum,
        'damage_type': {'id': damage_class, 'name': {1: 'Explosive', 2: 'Concussive',
                                                       3: 'Normal', 4: 'Ignore_Armor'}.get(damage_class, 'Normal')},
    }


class StockSpecCompareTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls._temp = tempfile.TemporaryDirectory(prefix='sc64-stock-spec-probe-')
        cls.probe = dump(ROOT, 'P3_FLAGS', Path(cls._temp.name) / 'p3')
        cls.units = {row['raw_id']: row for row in cls.probe['units']}

    @classmethod
    def tearDownClass(cls):
        cls._temp.cleanup()

    def make_row(self, unit, ref, weapon_facts):
        references = {ref['id']: ref}
        weapons = {item['id']: item for item in weapon_facts}
        return row_for('test-profile', self.probe, unit, references, weapons)

    def test_probe_reports_fixed_point_rates_not_one_or_two_pixel_guesses(self):
        self.assertEqual(self.probe['schema'], 2)
        for key in ('stock_profiles_sha256', 'stock_combat_sha256', 'stock_motion_sha256'):
            self.assertRegex(self.probe[key], r'^[0-9a-f]{64}$')
        unit = self.units[0]
        self.assertEqual(unit['motion'], {
            'available': 1,
            'runtime_speed_q10': 1638,
            'cardinal_steps_1024': 1638,
            'diagonal_steps_1024': 1158,
        })
        ref = unit_fact(0, 'Terran_Marine', 4.0, 0, 0, hp=40, minerals=50,
                        build_frames=360, supply=2)
        weapon = weapon_fact(0, 'Gauss_Rifle', 6, 1, 1, 15, 0, 128, 3)
        result = self.make_row(unit, ref, [weapon])
        self.assertEqual(result['movement_assessment'],
                         'fixedpoint_helper_matches_assumed_scale_and_clock')
        self.assertEqual(result['motion_helper_cardinal_steps_per_1024'], 1638)
        self.assertEqual(result['motion_helper_diagonal_steps_per_1024'], 1158)

    def test_goliath_air_damage_factor_is_not_misreported_as_missing_hits(self):
        unit = self.units[3]
        ref = unit_fact(3, 'Terran_Goliath', 4.57, 7, 8,
                        ground_hits=1, air_hits=1, hp=125, armor=1,
                        minerals=100, gas=50, build_frames=600, supply=4)
        weapon_facts = [
            weapon_fact(7, 'Twin_Autocannons', 12, 1, 1, 22, 0, 192, 3),
            weapon_fact(8, 'Hellfire_Missile_Pack', 10, 2, 2, 22, 0, 160, 1),
        ]
        result = self.make_row(unit, ref, weapon_facts)
        self.assertEqual(result['air_stock_helper_assessment'],
                         'stock_attack_helper_matches_reference')
        self.assertEqual(result['air_stock_helper_unit_max_hits'], 1)
        self.assertEqual(result['air_stock_helper_damage_factor'], 2)
        self.assertEqual(result['air_stock_helper_impact_count'], 2)
        self.assertEqual(result['air_stock_helper_upgrade_bonus'], 2)

    def test_siege_minimum_range_and_weapon_fields_are_reported_as_present(self):
        unit = self.units[30]
        ref = unit_fact(30, 'Terran_Siege_Tank_Siege_Mode', 0.0, 27, 130,
                        ground_hits=1, air_hits=0, hp=150, armor=1,
                        minerals=0, gas=0, build_frames=0)
        siege = weapon_fact(27, 'Arclite_Shock_Cannon', 70, 5, 1, 75,
                            64, 384, 1)
        none = weapon_fact(130, 'None', 0, 0, 0, 0, 0, 0, 5)
        result = self.make_row(unit, ref, [siege, none])
        self.assertEqual(result['ground_stock_helper_assessment'],
                         'stock_attack_helper_matches_reference')
        self.assertEqual(result['ground_stock_helper_attack_range_assessment'],
                         'range_boundary_helper_passed')
        self.assertEqual(result['ground_stock_helper_min_range_source_pixels'], 64)
        self.assertEqual(result['ground_stock_helper_half_scale_min_range'], 32)
        self.assertEqual(result['ground_stock_helper_damage_per_impact'], 70)
        self.assertEqual(result['ground_stock_helper_upgrade_bonus'], 5)
        self.assertEqual(result['ground_runtime_behavior_assessment'],
                         'stock_attack_fields; splash_and_mode_lifecycle_unmodeled')

    def test_zero_speed_unit_does_not_hide_a_nonzero_helper_rate(self):
        unit = dict(self.units[30])
        unit['motion'] = dict(unit['motion'], runtime_speed_q10=1,
                              cardinal_steps_1024=1)
        ref = unit_fact(30, 'Terran_Siege_Tank_Siege_Mode', 0.0, 27, 130,
                        hp=150, armor=1)
        weapons = [weapon_fact(27, 'Arclite_Shock_Cannon', 70, 5, 1, 75,
                               64, 384, 1),
                   weapon_fact(130, 'None', 0, 0, 0, 0, 0, 0, 5)]
        result = self.make_row(unit, ref, weapons)
        self.assertEqual(result['movement_assessment'], 'fixedpoint_helper_rate_difference')

    def test_no_weapon_reference_rejects_an_unexpected_available_helper(self):
        unit = self.units[0]
        ref = unit_fact(0, 'Terran_Marine', 4.0, 130, 130)
        none = weapon_fact(130, 'None', 0, 0, 0, 0, 0, 0, 5)
        result = self.make_row(unit, ref, [none])
        self.assertEqual(result['ground_stock_helper_assessment'],
                         'unexpected_stock_attack_helper_available')

    def test_stock_table_match_does_not_claim_profile_disabled_attack(self):
        unit = dict(self.units[3])
        unit['air_eligible'] = 0
        ref = unit_fact(3, 'Terran_Goliath', 4.57, 7, 8,
                        ground_hits=1, air_hits=1, hp=125, armor=1)
        weapons = [
            weapon_fact(7, 'Twin_Autocannons', 12, 1, 1, 22, 0, 192, 3),
            weapon_fact(8, 'Hellfire_Missile_Pack', 10, 2, 2, 22, 0, 160, 1),
        ]
        result = self.make_row(unit, ref, weapons)
        self.assertEqual(result['air_stock_helper_assessment'],
                         'stock_attack_helper_matches_reference')
        self.assertEqual(result['air_runtime_behavior_assessment'],
                         'profile_eligibility_disables_stock_attack')

    def test_stock_weapon_field_mismatch_is_reported(self):
        unit = dict(self.units[3])
        unit['air_stock'] = dict(unit['air_stock'], damage=11)
        ref = unit_fact(3, 'Terran_Goliath', 4.57, 7, 8,
                        ground_hits=1, air_hits=1, hp=125, armor=1)
        weapons = [
            weapon_fact(7, 'Twin_Autocannons', 12, 1, 1, 22, 0, 192, 3),
            weapon_fact(8, 'Hellfire_Missile_Pack', 10, 2, 2, 22, 0, 160, 1),
        ]
        result = self.make_row(unit, ref, weapons)
        self.assertEqual(result['air_stock_helper_assessment'],
                         'stock_attack_helper_mismatch')

    def test_range_boundary_assessment_includes_maximum_range(self):
        unit = dict(self.units[30])
        unit['ground_stock'] = dict(unit['ground_stock'], above_max_rejected=0)
        ref = unit_fact(30, 'Terran_Siege_Tank_Siege_Mode', 0.0, 27, 130,
                        hp=150, armor=1)
        weapons = [weapon_fact(27, 'Arclite_Shock_Cannon', 70, 5, 1, 75,
                               64, 384, 1),
                   weapon_fact(130, 'None', 0, 0, 0, 0, 0, 0, 5)]
        result = self.make_row(unit, ref, weapons)
        self.assertEqual(result['ground_stock_helper_attack_range_assessment'],
                         'range_boundary_helper_failed')

    def test_comparison_rejects_stale_pre_stock_helper_probes(self):
        with tempfile.TemporaryDirectory(prefix='sc64-stale-probe-') as temp:
            base = Path(temp)
            profile = base / 'probe' / 'old'
            profile.mkdir(parents=True)
            (profile / 'engine-facts.json').write_text(json.dumps({
                'schema': 1, 'units': [], 'engine_commit': 'old',
                'profile_variable': 'P3_FLAGS', 'defines': [], 'rules_sha256': 'old',
            }))
            facts = base / 'facts.json'
            facts.write_text(json.dumps({'units': [], 'weapons': [], 'provenance': {}}))
            with self.assertRaisesRegex(ValueError, 'stale probe schema'):
                compare(facts, base / 'probe', base / 'comparison')


if __name__ == '__main__':
    unittest.main()
