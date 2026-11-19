# SPDX-License-Identifier: GPL-3.0-or-later
import json
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from frontier_win_replay import CARTRIDGES, KEY_A, SETUP_UPDATE, case_inputs


class FrontierWinReplayTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.cases = json.loads((ROOT / "tests/frontier_win_inputs.json").read_text())

    def test_foundry_keeps_original_tapes_and_checkpoints(self):
        for case in self.cases:
            with self.subTest(case=case["name"]):
                self.assertEqual(case_inputs(case, "foundry"), (
                    [tuple(edge) for edge in case["button_edges"]], SETUP_UPDATE, case["end"]
                ))

    def test_translated_title_keeps_field_edges_and_checkpoints_aligned(self):
        for case in self.cases:
            original = case["button_edges"]
            start = next(i for i, (_, mask) in enumerate(original) if mask == KEY_A)
            old_launch = original[start][0]
            for cart in CARTRIDGES[1:]:
                with self.subTest(case=case["name"], cartridge=cart):
                    edges, setup, end = case_inputs(case, cart)
                    launch = next(i for i, (_, mask) in enumerate(edges) if mask == KEY_A)
                    shift = edges[launch][0] - old_launch
                    self.assertEqual(edges[launch - 1], (edges[launch][0] - 1, 0))
                    self.assertEqual([(tick - shift, mask) for tick, mask in edges[launch:]],
                                     [tuple(edge) for edge in original[start:]])
                    self.assertEqual((setup - shift, end - shift), (SETUP_UPDATE, case["end"]))
