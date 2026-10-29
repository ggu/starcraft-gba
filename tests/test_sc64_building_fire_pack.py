#!/usr/bin/env python3
"""Synthetic checks for the narrow fire timeline compiler.

SPDX-License-Identifier: GPL-3.0-or-later
"""
import unittest
import struct
from tools.sc64_building_fire_pack import timeline, parse_damage_lo

class FireTimelineTests(unittest.TestCase):
    def test_damage_lo_slot_indices_and_bounds(self):
        data = bytearray(struct.pack('<III', 1, 22, 12) + bytes([127, 127]) * 22)
        data[12 + 2 * 16:12 + 2 * 16 + 2] = bytes([252, 7])
        self.assertEqual(parse_damage_lo(data), [(-4, 7, 16)])
        for malformed in (data[:-1], struct.pack('<III', 1, 22, 0) + bytes(44)):
            with self.assertRaises(ValueError):
                parse_damage_lo(malformed)

    def test_loop_preserves_frame_and_duration(self):
        self.assertEqual(timeline(bytes([0, 3, 0, 5, 2, 0, 4, 0, 5, 3, 7, 0, 0]), 0), [(3, 2), (4, 3)])

    def test_rejects_non_looping_or_zero_wait(self):
        for data in (bytes([0, 0, 0, 5, 0, 7, 0, 0]), bytes([22])):
            with self.assertRaises(ValueError):
                timeline(data, 0)

    def test_rejects_branch_to_middle(self):
        with self.assertRaises(ValueError):
            timeline(bytes([0, 0, 0, 5, 2, 7, 3, 0]), 0)

if __name__ == '__main__':
    unittest.main()
