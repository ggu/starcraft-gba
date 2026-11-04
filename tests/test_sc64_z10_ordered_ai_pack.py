# SPDX-License-Identifier: GPL-3.0-or-later
"""Asset-free checks of bounded attack sequencing and loop-state refusal."""
import importlib.util
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('z10_pack', ROOT / 'tools/sc64_z10_ordered_ai_pack.py')
packer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(packer)


def program():
    # First group, clear, then a two-group loop, with a wait across the goto.
    rows = [('wait',[2]), ('attack_add',[1,65]), ('attack_prepare',[]),
            ('wait',[2]), ('attack_do',[]), ('attack_clear',[]),
            ('wait',[4]), ('attack_add',[2,70]), ('attack_add',[1,65]),
            ('attack_prepare',[]), ('wait',[2]), ('attack_do',[]), ('attack_clear',[]),
            ('wait',[4]), ('goto',[7])]
    return {'instructions':[{'offset':i,'name':name,'operands':args}
                            for i,(name,args) in enumerate(rows)]}


class OrderedProgramTest(unittest.TestCase):
    def test_order_and_loop(self):
        rows, loop = packer.extract(program())
        self.assertEqual(rows, [(5, ((65,1),)), (8, ((70,2),(65,1)))])
        self.assertEqual(loop, 1)

    def test_missing_clear_is_rejected(self):
        p=program();p['instructions'][12]['name']='wait';p['instructions'][12]['operands']=[0]
        with self.assertRaises(ValueError): packer.extract(p)

    def test_duplicate_type_rejected(self):
        p=program();p['instructions'][8]['operands']=[1,70]
        with self.assertRaises(ValueError): packer.extract(p)

    def test_unsupported_type_and_invalid_branch_rejected(self):
        p=program();p['instructions'][8]['operands']=[1,999]
        with self.assertRaises(ValueError): packer.extract(p)
        p=program();p['instructions'][-1]['operands']=[999]
        with self.assertRaises(ValueError): packer.extract(p)

    def test_wrong_provenance_rejected(self):
        with self.assertRaises(ValueError): packer.pack({'schema':1,'source':{}})

if __name__ == '__main__': unittest.main()
