#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Negative and positive cases for the strict replay observation comparator."""

from __future__ import annotations

import importlib.util
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "sc64_p8_arm_replay", ROOT / "tools" / "sc64_p8_arm_replay.py")
replay = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(replay)


def check(kind: str, tick: int, digest: int = 0, frame: int | None = None) -> str:
    frame = tick if frame is None else frame
    return f"P8 CHECK {kind} tick={tick} frame={frame} won={int(kind == 'won')} digest={digest}"


FULL = [check("periodic", 1000, 11), check("periodic", 2000, 22),
        check("periodic", 3000, 33), check("won", 3500, 44)]


class StrictCompareTest(unittest.TestCase):
    def assert_fails(self, a, b, from_tick=0):
        result = replay.compare(a, b, from_tick)
        self.assertFalse(replay.compare_ok(result), result)
        return result

    def test_identical_sequences_pass(self):
        self.assertTrue(replay.compare_ok(replay.compare(FULL, list(FULL), 0)))

    def test_missing_checkpoint_fails(self):
        result = self.assert_fails(FULL, [FULL[0], FULL[2], FULL[3]])
        self.assertEqual(result["missing"], [2000])

    def test_only_matching_final_fails(self):
        result = self.assert_fails(FULL, [FULL[3]])
        self.assertEqual(result["missing"], [1000, 2000, 3000])

    def test_interleaved_order_fails(self):
        result = self.assert_fails(FULL, [FULL[0], FULL[2], FULL[1], FULL[3]])
        self.assertIn("follows", result["errors"][0])

    def test_duplicate_tick_fails(self):
        result = self.assert_fails(FULL, [FULL[0], FULL[1], FULL[1], FULL[2], FULL[3]])
        self.assertIn("duplicate", result["errors"][0])

    def test_duplicate_tick_in_reference_fails(self):
        self.assert_fails([FULL[0], FULL[0]] + FULL[1:], FULL)

    def test_extra_checkpoint_fails(self):
        result = self.assert_fails(FULL, FULL[:3] + [check("periodic", 3200, 55), FULL[3]])
        self.assertEqual(result["extra"], [3200])

    def test_both_missing_digest_fail(self):
        missing = [row.replace(" digest=0", "") for row in [check("won", 900)]]
        self.assert_fails(missing, list(missing))

    def test_malformed_digest_fails(self):
        for value in ("", "12x", "4294967296", "-1"):
            row = f"P8 CHECK won tick=900 frame=900 won=1 digest={value}"
            self.assert_fails([row], [row])

    def test_p10_requires_complete_sentinel(self):
        old = replay.PREFIX
        try:
            replay.PREFIX = "P10"
            complete = ("P10 CHECK won digest=4294967295 tick=900 frame=900 screen=3 "
                        "won=1 lost=0 mission=29 faults=0 overmind_hp=0 zeratul_hp=3 "
                        "minerals=8 gas=7 switches=3 end=1")
            self.assertTrue(replay.compare_ok(replay.compare([complete], [complete], 0)))
            for row in (complete.replace(" end=1", ""),
                        complete.replace("digest=4294967295", "digest="),
                        complete.replace(" end=1", " end="),
                        complete.replace("digest=4294967295", "digest=12x"),
                        complete.replace(" gas=7", "")):
                self.assert_fails([row], [row])
        finally:
            replay.PREFIX = old

    def test_value_mismatch_fails(self):
        self.assert_fails(FULL, [FULL[0], check("periodic", 2000, 99)] + FULL[2:])

    def test_different_outcome_fails(self):
        self.assert_fails(FULL, FULL[:3] + [check("limit", 3500, 44)])

    def test_resume_cutoff_passes(self):
        resume = [check("resume", 1500, 7)] + FULL[1:]
        self.assertTrue(replay.compare_ok(replay.compare(FULL, resume, 1500)))

    def test_resume_missing_after_cutoff_fails(self):
        resume = [check("resume", 1500, 7), FULL[2], FULL[3]]
        result = self.assert_fails(FULL, resume, 1500)
        self.assertEqual(result["missing"], [2000])

    def test_observation_before_cutoff_fails(self):
        resume = [FULL[0], check("resume", 1500, 7)] + FULL[1:]
        self.assert_fails(FULL, resume, 1500)

    def test_unlabelled_cutoff_extra_fails(self):
        resume = [check("periodic", 1500, 7)] + FULL[1:]
        result = self.assert_fails(FULL, resume, 1500)
        self.assertEqual(result["extra"], [1500])

    def test_empty_candidate_fails(self):
        self.assert_fails(FULL, [])

    def test_only_final_in_both_fails(self):
        # A matching final record alone is not parity: the cadence check
        # expects checkpoints at 1000, 2000 and 3000 in each file.
        result = self.assert_fails([FULL[3]], [FULL[3]])
        self.assertTrue(any("cadence" in e for e in result["errors"]), result)

    def test_reference_missing_checkpoint_fails(self):
        thin = [FULL[0], FULL[2], FULL[3]]
        result = self.assert_fails(thin, list(thin))
        self.assertTrue(any("reference breaks" in e for e in result["errors"]), result)

    def test_unfinished_sequence_fails(self):
        self.assert_fails(FULL[:3], FULL[:3])

    def test_resume_without_cadence_fails(self):
        resume = [check("resume", 1500, 7), FULL[3]]
        a = [FULL[3]]
        result = self.assert_fails(a, resume, 1500)
        self.assertTrue(any("cadence" in e for e in result["errors"]), result)

    def test_resume_reference_passes(self):
        resume = [check("resume", 1500, 7)] + FULL[1:]
        self.assertTrue(replay.compare_ok(replay.compare(resume, list(resume), 1500)))

    def test_short_run_with_only_final_passes(self):
        short = [check("won", 900, 5)]
        self.assertTrue(replay.compare_ok(replay.compare(short, list(short), 0)))


if __name__ == "__main__":
    unittest.main()
