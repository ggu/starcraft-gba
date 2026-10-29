#!/usr/bin/env python3
"""Synthetic IScript tests. SPDX-License-Identifier: GPL-3.0-or-later"""
import unittest
from tools.sc64_order_marker_pack import compile_timeline

class TimelineTests(unittest.TestCase):
    def test_five_frame_script(self):
        data = b''.join(bytes([0, i, 0, 5, 1]) for i in range(5)) + bytes([45])
        self.assertEqual(compile_timeline(data, 0), [(i, 1) for i in range(5)])

    def test_reject_invalid_or_unbounded(self):
        for script in (bytes([5,1,45]), bytes([0,0,0,5,0,45]), bytes([7,0,0]), bytes([0,0,0])):
            with self.assertRaises(ValueError):
                compile_timeline(script, 0)

if __name__ == '__main__':
    unittest.main()
