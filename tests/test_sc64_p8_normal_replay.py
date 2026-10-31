# SPDX-License-Identifier: GPL-3.0-or-later
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import sc64_p8_normal_replay as replay
import sc64_z3_controller_replay as scheduler


class ResumeInputTest(unittest.TestCase):
    def test_resume_reestablishes_held_key_then_replays_future_transition(self):
        self.assertEqual(replay.resume_inputs([(0, 1), (5, 16), (9, 0), (12, 4)], 7),
                         [(0, 512), (1, 0), (2, 16), (4, 0), (7, 4)])

    def test_resume_with_neutral_held_has_no_duplicate_tick(self):
        events = replay.resume_inputs([(0, 1), (5, 0), (7, 4)], 6)
        self.assertEqual(events, [(0, 512), (1, 0), (3, 4)])
        self.assertEqual(scheduler.make_schedule_pages({'inputs': events}, 10, 256)[0]['events'], events)

    def test_wrong_save_boundary_fails_before_emulation(self):
        with self.assertRaises(ValueError):
            replay.validate_save_boundary({'save': {'resume_call': 32445}}, 32444)
        with self.assertRaises(ValueError):
            replay.validate_save_boundary({'save': None}, 32445)
        replay.validate_save_boundary({'save': {'resume_call': 32445}}, 32445)

    def test_forged_saved_fingerprint_fails_closed(self):
        saved = {'frame': 12, 'fingerprint': 'a' * 64}
        restored = {'frame': 12, 'fingerprint': 'b' * 64}
        with self.assertRaises(RuntimeError):
            replay.verify_restored(saved, restored)
        with self.assertRaises(ValueError):
            replay.verify_restored({'frame': 12, 'fingerprint': 'invalid'}, restored)

    def test_mismatched_saved_frame_fails_closed(self):
        with self.assertRaises(RuntimeError):
            replay.verify_restored({'frame': 12, 'fingerprint': 'a' * 64},
                                   {'frame': 13, 'fingerprint': 'a' * 64})


if __name__ == '__main__':
    unittest.main()
