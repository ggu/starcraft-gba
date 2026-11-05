# SPDX-License-Identifier: GPL-3.0-or-later
import struct
import unittest
from tools import sc64_guardian_cocoon_bank as bank

class CocoonTimelineTest(unittest.TestCase):
    @staticmethod
    def program():
        # Underlay, frame 1, fixed wait, frame 2, random wait, repeat.
        return bytes([9,12,0,0,42, 0,1,0, 5,2, 0,2,0, 6,2,5, 7,5,0])
    def test_bounded_constant_time_table_is_repeatable(self):
        program=self.program()
        first=bank.compile_timeline(program,0,3,0,5)
        self.assertEqual(len(first),751)
        self.assertEqual(first[:3],bytes([1,1,1]))
        self.assertEqual(first,bank.compile_timeline(program,0,3,0,5))
        self.assertNotEqual(first,bank.compile_timeline(program,0,3,1,5))
        self.assertEqual(set(first),{1,2})
    def test_rejects_unsupported_and_out_of_range(self):
        with self.assertRaisesRegex(ValueError,'unsupported reachable'):
            bank.compile_timeline(bytes([9,12,0,0,42,22]),0,3,0,5)
        with self.assertRaisesRegex(ValueError,'source frame'):
            bank.compile_timeline(bytes([9,12,0,0,42,0,4,0]),0,3,0,5)
        with self.assertRaisesRegex(ValueError,'underlay'):
            bank.compile_timeline(bytes([9,13,0,0,42]),0,3,0,5)
        with self.assertRaisesRegex(ValueError,'non-yielding'):
            bank.compile_timeline(bytes([9,12,0,0,42,7,5,0]),0,3,0,5)

if __name__ == '__main__': unittest.main()
