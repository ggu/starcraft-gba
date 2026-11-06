#!/usr/bin/env python3
"""Asset-free restriction and reference-precedence tests."""
import struct
import sys
from pathlib import Path
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from tools.sc64_protoss_upgrade_policy import policy, decode_restrictions


def section(count):
    data = bytearray(38 * count)
    for ident in (5, 6, 13, 14, 15, 33):
        if ident < count:
            data[24 * count + ident] = 1 if ident == 33 else 3
            for owner in range(8): data[26 * count + owner * count + ident] = 1
    return data

class PolicyTest(unittest.TestCase):
    def test_expansion_overrides_base(self):
        base=section(46); expanded=section(61)
        expanded[7*61+6]=0; expanded[26*61+7*61+6]=0
        settings=bytearray(598)
        for i in (5,6,13,14,15,33): settings[i]=1
        sections={b'VER ':struct.pack('<H',59),b'UPGS':settings,b'UPGR':base,b'PTEC':bytearray(38*24),b'PUPx':expanded}
        upgrade,tech=policy(sections)
        self.assertEqual(upgrade[0][1],(3,0)); self.assertEqual(upgrade[7][1],(0,0)); self.assertEqual(tech[0][0],(0,0))
        del sections[b'PUPx']; self.assertEqual(policy(sections)[0][7][1],(3,0))
        settings[5]=0
        with self.assertRaises(ValueError): policy(sections)
    def test_reject_invalid(self):
        data=section(46);data[26*46+5]=2
        with self.assertRaises(ValueError): decode_restrictions(data,46,(5,))
        with self.assertRaises(ValueError): decode_restrictions(bytes(2),46,(5,))
        with self.assertRaises(ValueError): policy({b'VER ':struct.pack('<H',205)})

if __name__=='__main__': unittest.main()
