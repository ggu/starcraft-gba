# SPDX-License-Identifier: GPL-3.0-or-later
"""Disjoint pacing counters: inclusive timers and advancing-tick denominator."""
import sys
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from sc64_custom_full_match import disjoint_pace_budget, pace_budget_report, MatchError, TWO_VBLANK_CYCLES


def row(**changes):
    data = dict(world_advanced=True, host_tick=5, cycles=1000, demo_update_cycles=400,
                gfx_draw_cycles=500, sc64_orders_cycles=200, sc64_sight_cycles=50,
                sc64_other_cycles=100, sc64_background_cycles=100,
                sc64_unit_draw_cycles=200, sc64_hud_cycles=150)
    data.update(changes)
    return data


class BudgetTest(unittest.TestCase):
    def test_nested_counters_are_disjoint(self):
        b = disjoint_pace_budget([row()])
        self.assertEqual(b['mean_disjoint_cycles'], dict(orders=200, sight=50,
            other_simulation=100, remaining_simulation=50, map_background=100,
            actors=200, hud=150, remaining_render=50, interval_less_update_and_draw=100))
        self.assertEqual(sum(b['mean_disjoint_cycles'].values()), 1000)
        self.assertEqual(b['two_vblank_cycles'], 561792)
        self.assertEqual(TWO_VBLANK_CYCLES, 561792)
        self.assertAlmostEqual(b['nominal_30_fps_cycles'], 16777216/30)

    def test_paused_sample_does_not_use_stale_counters(self):
        b = disjoint_pace_budget([row(), row(world_advanced=False, cycles=1)])
        self.assertEqual(b['samples'], 1)
        self.assertEqual(b['mean_total_cycles'], 1000)
        mixed = pace_budget_report([row(), row(cycles=1200), row(world_advanced=False)])
        self.assertEqual(mixed['samples'], 2)
        self.assertEqual(mixed['mean_total_cycles'], 1100)
        with self.assertRaisesRegex(MatchError, 'no simulation updates'):
            disjoint_pace_budget([row(world_advanced=False)])

    def test_optional_budget_preserves_bad_tick_diagnostics(self):
        result = pace_budget_report([row(), row(host_tick=9, cycles=899)])
        self.assertFalse(result['available'])
        self.assertEqual(result['invalid_samples'], 1)
        self.assertEqual(result['offending_samples'][0]['host_tick'], 9)
        self.assertIn('inconsistent', result['offending_samples'][0]['reason'])
        self.assertTrue(pace_budget_report([row()])['available'])

    def test_missing_invalid_and_inconsistent(self):
        r = row(); del r['world_advanced']
        with self.assertRaisesRegex(MatchError, 'world_advanced'):
            disjoint_pace_budget([r])
        r = row(); del r['sc64_sight_cycles']
        with self.assertRaisesRegex(MatchError, 'missing counters'):
            disjoint_pace_budget([r])
        for changes in ({'cycles': -1}, {'sc64_orders_cycles': '1'},
                        {'sc64_orders_cycles': 251}, {'sc64_hud_cycles': 201},
                        {'cycles': 899}):
            with self.subTest(changes=changes), self.assertRaises(MatchError):
                disjoint_pace_budget([row(**changes)])


if __name__ == '__main__':
    unittest.main()
