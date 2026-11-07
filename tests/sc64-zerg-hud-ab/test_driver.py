# SPDX-License-Identifier: GPL-3.0-or-later
import hashlib
import json
from pathlib import Path
import sys
import tempfile
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'tools'))
import sc64_hud_zerg_lobby_plan as plan
import sc64_zerg_hud_pair as pair
class Lobby(unittest.TestCase):
    def test_prefix_and_unchanged_gameplay_edges(self):
        with tempfile.TemporaryDirectory() as folder:
            source=Path(folder)/'tape.jsonl'
            rows=[{'kind':'input','call_index':i,'held':k} for i,k in enumerate(plan.ORIGINAL)]
            rows += [{'kind':'input','call_index':100,'held':1},{'kind':'input','call_index':101,'held':0}]
            data=('\n'.join(json.dumps(x) for x in rows)+'\n').encode();source.write_bytes(data)
            old=plan.EXPECTED;plan.EXPECTED=hashlib.sha256(data).hexdigest()
            try:
                result=plan.derive(source)
                self.assertEqual(result['transitions'][:24],list(enumerate(plan.REPLACEMENT)))
                self.assertEqual(result['transitions'][-2:],[(108,1),(109,0)])
                self.assertEqual(result['unverified_save_tick'],3056)
                source.write_bytes(data+b' ')
                with self.assertRaises(ValueError):plan.derive(source)
            finally:plan.EXPECTED=old
    def test_raw_pixel_bounds_and_conditions(self):
        a=bytes(38400);b=bytearray(a);b[108*240+59]=1;b[115*240+120]=2
        self.assertEqual(pair.pixel_difference(a,a),{'count':0,'bbox':None})
        self.assertEqual(pair.pixel_difference(a,b),{'count':2,'bbox':[59,108,120,115]})
        ref=[{'screen':1,'frame':20},{'screen':1,'frame':20}]
        loaded=[dict(ref[0]),dict(ref[1])]
        baseline=[(4,'p','palette'),(4,'p','palette')]
        changed=[(20,'other','palette'),(20,'other','palette')]
        checks=pair.compare_fresh_reference(ref,baseline,loaded,changed)
        self.assertTrue(checks['restored_pair_view_equal'])
        self.assertTrue(checks['variants'][0]['named_state_equal'])
        self.assertFalse(checks['variants'][0]['pixel_equal'])
        self.assertTrue(checks['variants'][0]['display_except_page_equal'])
if __name__=='__main__':unittest.main()
