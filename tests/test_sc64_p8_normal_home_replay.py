# SPDX-License-Identifier: GPL-3.0-or-later
"""Asset-free admission checks for the P8 physical replay plan."""

import json
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from sc64_p8_normal_home_replay import parse_plan, validate_rows


class ReplayPlanTest(unittest.TestCase):
    def setUp(self):
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        self.tape = Path(temp.name) / "tape.jsonl"
        self.log = Path(temp.name) / "host.log"
        self.log.write_text("2 briefing 0 2\n4 briefing 1 2\n")
        self.rows = [
            dict(kind="metadata", mission=27, schema=1),
            dict(kind="input", call_index=0, held=1, pressed=1, frame_after=0),
            dict(
                kind="checkpoint",
                label="final",
                call_index=20,
                frame=15,
                screen=3,
                won=1,
                lost=0,
                fault=[0, 0, 0],
                outcome=1,
                triggers_done=[16, 0, 0],
                stasis_hp=-1,
                fenix=[0, 240, 240],
                hyperion=[0, 850, 0],
            ),
        ]

    def parse(self):
        self.tape.write_text("".join(json.dumps(q) + "\n" for q in self.rows))
        return parse_plan(self.tape, self.log)

    def test_plan_preserves_call_indices(self):
        plan, keys, checks, edges, briefs = self.parse()
        self.assertEqual(plan["stop"], 20)
        self.assertEqual(keys, {0: 1})
        self.assertEqual(list(checks), [20])
        self.assertEqual(edges[0]["frame_after"], 0)
        self.assertEqual(briefs, {2: (0, 2), 4: (1, 2)})

    def test_missing_briefing_page_fails(self):
        self.log.write_text("2 briefing 0 2\n")
        with self.assertRaisesRegex(ValueError, "incomplete"):
            self.parse()

    def test_opposing_buttons_fail(self):
        self.rows[1].update(held=48, pressed=48)
        with self.assertRaisesRegex(ValueError, "opposing"):
            self.parse()

    def test_dialogue_metadata_is_bounded(self):
        valid = dict(trigger=2, action=3, page_count=1, string_id=8)
        validate_rows([valid])
        for k, value in (("page_count", 0), ("trigger", 8), ("string_id", 0)):
            with self.subTest(k=k), self.assertRaisesRegex(ValueError, "metadata"):
                validate_rows([dict(valid, **{k: value})])
