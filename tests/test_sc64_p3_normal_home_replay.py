# SPDX-License-Identifier: GPL-3.0-or-later
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from sc64_p3_normal_home_replay import FIELDS, load_plan


class P3PlanTest(unittest.TestCase):
    def fixture(self):
        rows = []
        before = 0
        for i, mask in enumerate((1, 0, 16, 0, 1, 0, 1, 0, 1, 0), 1):
            rows.append(
                dict(kind="input", host_tick=i, held=mask, pressed=mask & ~before)
            )
            before = mask
        rows.insert(
            8, dict(dict.fromkeys(FIELDS, 0), kind="start", host_tick=8, screen=1)
        )
        rows.append(
            dict(
                dict.fromkeys(FIELDS, 0),
                kind="won",
                host_tick=10,
                won=1,
                screen=3,
                p3_outcome=1,
                p3_sequence=5,
                campaign_phase=3,
                objective_flags=1,
            )
        )
        return dict(
            schema=1,
            mission=22,
            policy="protoss-stock-economy-cannons-assault",
            events=rows,
        ), "6 briefing 0 1\n8 0 2 0 5 1 0 1\n"

    def test_call_mapping(self):
        stop, keys, checks, briefs, modal = load_plan(*self.fixture())
        self.assertEqual((stop, list(checks), briefs), (10, [8, 10], {6: (0, 1)}))
        self.assertEqual((keys[8], keys[9], modal[8][1:5]), (1, 0, (2, 0, 5, 1)))

    def test_rejects_wrong_modal_edges(self):
        trace, pages = self.fixture()
        with self.assertRaisesRegex(ValueError, "authored P3 input"):
            load_plan(trace, pages.replace("5 1 0 1", "5 1 0 8"))

    def test_rejects_nonterminal_objective(self):
        trace, pages = self.fixture()
        trace["events"][-1]["p3_sequence"] = 4
        with self.assertRaisesRegex(ValueError, "strict terminal"):
            load_plan(trace, pages)
