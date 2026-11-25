#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Small validation tests for the linked ordinary-input replay."""
import json
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import sc64_protoss_linked_replay as replay


class TapeValidation(unittest.TestCase):
    def write(self, rows):
        path = Path(self.tmp.name) / 'tape.jsonl'
        path.write_text(''.join(json.dumps(row) + '\n' for row in rows))
        return path

    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.rows = [dict(kind='metadata', mission=28),
                     dict(kind='input', call_index=54, held=1, pressed=1),
                     dict(kind='input', call_index=55, held=0, pressed=0),
                     dict(kind='checkpoint', call_index=56, label='final', won=1)]

    def test_splice_and_duplicate_checkpoint(self):
        rows = self.rows + [dict(kind='checkpoint', call_index=56, label='final', won=1)]
        events, checks = replay.load_tape(self.write(rows), 28, 50)
        self.assertEqual(events, [(0, 0), (2, replay.KEY_L), (3, 0), (4, 1), (5, 0)])
        self.assertEqual(len(checks), 2)

    def test_rejects_wrong_mission_and_missing_launch(self):
        with self.assertRaisesRegex(ValueError, 'mission 29'):
            replay.load_tape(self.write(self.rows), 29, 50)
        with self.assertRaisesRegex(ValueError, 'launch transition'):
            replay.load_tape(self.write([self.rows[0], self.rows[-1]]), 28, 50)

    def test_rejects_unordered_checkpoints(self):
        with self.assertRaisesRegex(ValueError, 'unordered'):
            replay.load_tape(self.write(self.rows + [dict(kind='checkpoint', call_index=55)]), 28, 50)

    def test_checkpoint_comparison_rejects_missing_and_different(self):
        replay.compare_checkpoint({'frame': 1, 'bank': [3, 4]}, {'frame': 1, 'bank': [3, 4]}, 56)
        with self.assertRaisesRegex(ValueError, 'missing fields'):
            replay.compare_checkpoint({'frame': 1}, {'frame': 1, 'bank': [3, 4]}, 56)
        with self.assertRaisesRegex(ValueError, 'mismatch'):
            replay.compare_checkpoint({'frame': 1, 'bank': [3, 5]}, {'frame': 1, 'bank': [3, 4]}, 56)


if __name__ == '__main__':
    unittest.main()
