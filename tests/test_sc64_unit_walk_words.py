# SPDX-License-Identifier: GPL-3.0-or-later
import importlib.util
from pathlib import Path
import sys
import unittest
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
spec=importlib.util.spec_from_file_location('walk_words',ROOT/'tools/sc64_unit_walk_words.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)

class WalkWordsTest(unittest.TestCase):
    def test_selected_full_pose_and_unselected_fallback(self):
        facing='static const unsigned char facing_pixels[2] = {\n0x01,0x00,\n};\n'
        frame='{1,2,2,1,4,4,facing_pixels}'
        walk=('static const Sc64SpriteFrame pose[32] = {\n'+','.join([frame]*32)+'\n};\n'
              'static const Sc64UnitWalkStep sc64_unit_walk_i1_steps[1] = {\n{0u,pose}\n};\n'
              'case 37: return &sc64_unit_walk_i1;\n')
        output=m.derive(walk,facing,(37,))
        self.assertIn('case 37:',output)
        self.assertIn('default: return 0;',output)
        self.assertEqual(output.count('static const GbaSpriteWordFrame sc64_walk_word_0 ='),1)
        self.assertIn('0x00000001u,0x000000ffu',output)
        self.assertEqual(output.count('&sc64_walk_word_0'),32)
        with self.assertRaises(ValueError):m.derive(walk,facing,(38,))
        with self.assertRaises(ValueError):m.derive(walk.replace(frame,'{1,2,3,1,4,4,facing_pixels}',1),facing,(37,))

if __name__=='__main__':unittest.main()
