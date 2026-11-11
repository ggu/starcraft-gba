# SPDX-License-Identifier: GPL-3.0-or-later
import unittest
from tools.sc64_mid_zerg_permissions import render
from tools.sc64_t5_availability import effective_bits

class MidZergPermissionsTest(unittest.TestCase):
    def test_rows_are_stable_and_dimensions_checked(self):
        rows=[{'mission':i,'owners':[0]*12,'races':[0]*12,'bits':bytes(342)} for i in (16,17)]
        a=render(rows)
        self.assertEqual(a,render(rows));self.assertIn('sc64_mid_zerg_permissions[2]',a)
        with self.assertRaises(ValueError): render(rows[::-1])
        rows[0]['bits']=bytes(341)
        with self.assertRaises(ValueError): render(rows)
    def test_effective_bits_rejects_truncated_puni(self):
        with self.assertRaises(ValueError): effective_bits(bytes(5699))
if __name__=='__main__': unittest.main()
