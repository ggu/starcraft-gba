"""Asset-free operand/schema checks for the private late-campaign auditor."""
from __future__ import annotations

import importlib.util
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    'sc64_terran_late_audit', ROOT / 'tools/sc64_terran_late_audit.py')
assert SPEC and SPEC.loader
audit = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(audit)


class LateCampaignAuditTests(unittest.TestCase):
    def test_exact_mission_ids_are_stable_path_derived(self):
        self.assertEqual([audit.global_id(path) for path in audit.PATHS],
                         [10, 11, 12, 13])
        self.assertEqual(audit.global_id('008/00F'), 9)
        with self.assertRaises(ValueError):
            audit.global_id('008/014')

    def test_runtime_pool_classification_includes_raw195_building(self):
        counts = __import__('collections').Counter({0: 110, 195: 1, 109: 2,
                                                    176: 3, 188: 1})
        categories = audit.placement_counts(counts, 2)
        self.assertEqual(categories['ordinary_units_source_class'], 110)
        self.assertEqual(categories['buildings_runtime_class'], 3)
        self.assertEqual(categories['runtime_units_if_raw195_is_building'], 110)
        self.assertEqual(categories['minerals'], 3)
        self.assertEqual(categories['geysers'], 1)
        self.assertEqual(categories['start_markers'], 2)

    def test_create_count_uses_opcode11_modifier_not_player_selector(self):
        self.assertEqual(audit.create_instance_count({
            'opcode': 11, 'group1': 19, 'group2': 0, 'modifier': 0}), 1)
        self.assertEqual(audit.create_instance_count({
            'opcode': 11, 'group1': 3, 'group2': 1, 'modifier': 4}), 4)
        with self.assertRaises(ValueError):
            audit.create_instance_count({'opcode': 43, 'modifier': 0})
        with self.assertRaises(ValueError):
            audit.create_instance_count({'opcode': 11, 'modifier': -1})

    def test_native_comparator_codes_include_exactly(self):
        self.assertEqual(audit.n64_comparison_meaning(0), 'at_least')
        self.assertEqual(audit.n64_comparison_meaning(1), 'at_most')
        self.assertEqual(audit.n64_comparison_meaning(10), 'exactly')
        self.assertIsNone(audit.n64_comparison_meaning(2))

    def test_selector26_uses_observed_reciprocal_relation_predicate(self):
        self.assertFalse(audit.n64_selector26_matches(0, 0, 0, 0))
        self.assertFalse(audit.n64_selector26_matches(0, 1, 2, 2))
        self.assertTrue(audit.n64_selector26_matches(0, 1, 2, 1))
        self.assertTrue(audit.n64_selector26_matches(0, 1, 1, 2))
        self.assertTrue(audit.n64_selector26_matches(0, 1, 0, 0))
        with self.assertRaises(ValueError):
            audit.n64_selector26_matches(8, 0, 0, 0)


if __name__ == '__main__':
    unittest.main()
