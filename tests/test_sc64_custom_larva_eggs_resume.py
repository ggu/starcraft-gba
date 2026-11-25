#!/usr/bin/env python3
"""Asset-free checks of the Larva and Egg resume comparison."""
from __future__ import annotations

import copy
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import sc64_custom_larva_eggs_resume as resume


def world(kind, frame, digest, **changes):
    row = {"kind": kind, "frame": frame, "digest": digest, "human_eggs": 3, "bot_eggs": 3,
           "human_queued": 0, "bot_queued": 0, "human_larva": 0, "bot_larva": 0}
    row.update(changes)
    return row


def runs():
    saved = {"saved": world("saved", 327, "c12e0242"),
             "continued": world("continued", 1227, "da45f9a4", human_eggs=0, bot_eggs=1, human_larva=2)}
    resumed = {"loaded": world("loaded", 327, "c12e0242"),
               "continued": world("continued", 1227, "da45f9a4", human_eggs=0, bot_eggs=1, human_larva=2)}
    return saved, resumed


class Compare(unittest.TestCase):
    def test_identical_runs_pass(self):
        line = resume.compare(*runs())
        self.assertIn("3 human and 3 computer Eggs", line)
        self.assertIn("update 1227", line)

    def rejects(self, edit, text):
        saved, resumed = copy.deepcopy(runs())
        edit(saved, resumed)
        with self.assertRaises(SystemExit) as raised:
            resume.compare(saved, resumed)
        self.assertIn(text, str(raised.exception))

    def test_rejects_a_different_restore_or_continuation(self):
        self.rejects(lambda s, r: r["loaded"].update(digest="00000000"), "restored world differs")
        self.rejects(lambda s, r: r["loaded"].update(bot_larva=1), "restored world differs")
        self.rejects(lambda s, r: r["continued"].update(digest="00000000"), "continuation differs")
        self.rejects(lambda s, r: r["continued"].update(frame=1226), "continuation differs")

    def test_rejects_a_record_without_eggs_on_both_sides(self):
        def one_side(s, r):
            s["saved"].update(bot_eggs=1)
            r["loaded"].update(bot_eggs=1)
        self.rejects(one_side, "too few Eggs")
        self.rejects(lambda s, r: (s["saved"].update(human_queued=1), r["loaded"].update(human_queued=1)),
                     "serial queue entry")

    def test_rejects_missing_records_and_no_progress(self):
        self.rejects(lambda s, r: s.pop("continued"), "expected saved+continued")
        self.rejects(lambda s, r: r.pop("loaded"), "expected saved+continued")
        self.rejects(lambda s, r: (s["continued"].update(frame=327), r["continued"].update(frame=327)),
                     "did not advance")


if __name__ == "__main__":
    unittest.main()
