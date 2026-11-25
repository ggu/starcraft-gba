# SPDX-License-Identifier: GPL-3.0-or-later
import sys
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from sc64_z10_fifo_normal_replay import compare_checkpoint


class Z10CheckpointTest(unittest.TestCase):
    def test_exact_match_missing_and_changed(self):
        expected = {'call_index': 42, 'frame': 7, 'bank': [8, 2], 'supply': [10, 20]}
        compare_checkpoint(expected, {'frame': 7, 'bank': [8, 2], 'supply': [10, 20]})
        with self.assertRaisesRegex(ValueError, 'bank'):
            compare_checkpoint(expected, {'frame': 7, 'bank': [8, 3]})
        with self.assertRaisesRegex(ValueError, 'fault'):
            compare_checkpoint(expected, {'frame': 7, 'fault': 0})

if __name__ == '__main__': unittest.main()
