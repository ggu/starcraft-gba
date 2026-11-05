# SPDX-License-Identifier: GPL-3.0-or-later
import struct
import unittest
from tools.sc64_early_zerg_permissions import decode, render_header


def section(tag, data):
    return tag + struct.pack('<I', len(data)) + data


def chk(puni):
    return (section(b'PUNI', puni) + section(b'OWNR', bytes(range(12))) +
            section(b'SIDE', bytes([1] * 12)) + section(b'DIM ', b'\x40\0\x40\0'))


class EarlyPermissionsTest(unittest.TestCase):
    def test_player_and_default_selectors_and_final_bit(self):
        source = bytearray(5700)
        source[0] = 1
        source[1] = 1
        source[2964 + 1] = 1  # Default raw1 is zero: disables player raw1.
        source[2736 + 227] = 1
        source[2964 + 455] = 1  # Player1/raw227 inherits default availability.
        source[2735] = 1  # Player11/raw227, final bit, explicit availability.
        row = decode(chk(source))
        expected = bytearray(342)
        expected[0] = 1
        expected[455 >> 3] |= 1 << (455 & 7)
        expected[-1] |= 1 << 7
        self.assertEqual(bytes(expected), row['effective'])
        self.assertEqual(bytes(range(12)), row['owners'])
        self.assertEqual(bytes([1] * 12), row['races'])
        self.assertEqual(bytes(source), row['puni'])

    def test_missing_wrong_duplicate_and_truncated_inputs_reject(self):
        valid = chk(bytes(5700))
        for data in (valid[:-1], valid + section(b'PUNI', bytes(5700)),
                     chk(bytes(5699)), section(b'PUNI', bytes(5700))):
            with self.subTest(length=len(data)):
                with self.assertRaises(ValueError):
                    decode(data)

    def test_header_rejects_reordered_or_incomplete_mission_set(self):
        base = decode(chk(bytes(5700)))
        rows = [dict(base, mission=i) for i in [0, 1, 14, 15]]
        self.assertIn('sc64_early_zerg_permissions[4]', render_header(rows))
        for bad in (rows[:3], rows[::-1], [dict(row, effective=b'') for row in rows]):
            with self.assertRaises(ValueError):
                render_header(bad)
