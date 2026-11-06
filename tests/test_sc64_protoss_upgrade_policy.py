#!/usr/bin/env python3
"""Asset-free restriction and reference-precedence tests."""
import struct
import sys
from pathlib import Path
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from tools.sc64_protoss_upgrade_policy import policy, decode_restrictions, validate_catalog, collect, emit, MISSIONS


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
        upgrade,tech,cost=policy(sections)
        self.assertEqual(upgrade[0][1],(3,0)); self.assertEqual(upgrade[7][1],(0,0)); self.assertEqual(tech[0][0],(0,0))
        del sections[b'PUPx']; self.assertEqual(policy(sections)[0][7][1],(3,0))
        settings[5]=0
        self.assertEqual(policy(sections)[2],[0,1,1,1,1,1])
    def test_cost_selectors_apply_sequentially(self):
        base = bytearray(598)
        expanded = bytearray(794)
        for ident in (5, 6, 13, 14, 15, 33):
            base[ident] = expanded[ident] = 1
        sections = {b'VER ': struct.pack('<H', 59), b'UPGS': base,
                    b'UPGx': expanded, b'UPGR': section(46),
                    b'PTEC': bytes(38 * 24)}
        self.assertEqual(policy(sections)[2], [1] * 6)
        base[5] = 0
        self.assertEqual(policy(sections)[2], [0, 1, 1, 1, 1, 1])
        base[5], expanded[5] = 1, 0
        self.assertEqual(policy(sections)[2], [0, 1, 1, 1, 1, 1])
        base[5] = 2
        with self.assertRaises(ValueError): policy(sections)
        base[5], expanded[5] = 1, 2
        with self.assertRaises(ValueError): policy(sections)

    def test_catalog_identity_and_entry_coverage(self):
        catalog=validate_catalog(list(reversed(MISSIONS)))
        self.assertEqual(catalog[('T',1)], next(p for n,p,_ in MISSIONS if n=='T1'))
        self.assertEqual(catalog[('P',10)], next(p for n,p,_ in MISSIONS if n=='P10'))
        with self.assertRaises(ValueError): validate_catalog(MISSIONS[:-1])
        with self.assertRaises(ValueError): validate_catalog(MISSIONS+[MISSIONS[0]])
        duplicate_path=list(MISSIONS); duplicate_path[-1]=(duplicate_path[-1][0],MISSIONS[0][1],'')
        with self.assertRaises(ValueError): validate_catalog(duplicate_path)
        class Entry:
            def __init__(self,path): self.path=path
        class Archive:
            def __init__(self,paths): self.paths=paths
            def entries(self): return [Entry(p) for p in self.paths]
            def read(self,entry):
                settings=bytearray(598)
                for i in (5,6,13,14,15,33): settings[i]=1
                chunks={b'VER ':struct.pack('<H',59),b'UPGS':settings,b'UPGR':section(46),b'PTEC':bytes(38*24)}
                return b''.join(tag+struct.pack('<I',len(payload))+payload for tag,payload in chunks.items())
        paths=list(catalog.values())
        with self.assertRaises(ValueError): collect(Archive(paths[:-1]),catalog)
        with self.assertRaises(ValueError): collect(Archive(paths+[paths[0]]),catalog)
        rows=collect(Archive(paths),catalog)
        result=emit(rows)
        self.assertLess(result.index('T1 '),result.index('Z1 '))
        self.assertLess(result.index('Z1 '),result.index('P1 '))
        self.assertIn('[3][10][8][6][2]',result)
    def test_reject_invalid(self):
        data=section(46);data[26*46+5]=2
        with self.assertRaises(ValueError): decode_restrictions(data,46,(5,))
        with self.assertRaises(ValueError): decode_restrictions(bytes(2),46,(5,))
        with self.assertRaises(ValueError): policy({b'VER ':struct.pack('<H',205)})

if __name__=='__main__': unittest.main()
