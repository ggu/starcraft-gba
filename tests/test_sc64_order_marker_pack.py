#!/usr/bin/env python3
"""Synthetic IScript tests. SPDX-License-Identifier: GPL-3.0-or-later"""
import unittest
from tools.sc64_order_marker_pack import compile_timeline, nearest_ui_color

class TimelineTests(unittest.TestCase):
    def test_bright_green_uses_opaque_ui_green(self):
        # Authored RGB fixture, not a ROM-derived sample.
        ui = [(0, 0, 0)] * 32
        ui[7] = (57, 230, 99)  # C_GREEN
        ui[8] = (16, 82, 41)   # C_GREEN_DARK
        self.assertEqual(nearest_ui_color((16, 239, 24), ui), 7)
        self.assertNotEqual(nearest_ui_color((0, 0, 0), ui), 0)

    def test_five_frame_script(self):
        data = b''.join(bytes([0, i, 0, 5, 1]) for i in range(5)) + bytes([45])
        self.assertEqual(compile_timeline(data, 0), [(i, 1) for i in range(5)])

    def test_reject_invalid_or_unbounded(self):
        for script in (bytes([5,1,45]), bytes([0,0,0,5,0,45]), bytes([7,0,0]), bytes([0,0,0])):
            with self.assertRaises(ValueError):
                compile_timeline(script, 0)

if __name__ == '__main__':
    unittest.main()
