"""Pure P7 physical transition parser checks; no emulator or SRAM."""
import copy
import json
from pathlib import Path
import tempfile
import unittest
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import sc64_p7_physical_replay as replay


def rows():
    return [
        {'kind':'metadata','schema':1,'mission':26,'profile':'PROTOSS_P7_CANONICAL_FLAGS'},
        {'kind':'input','call_index':0,'held':1,'pressed':1,'frame_before':0,'frame_after':0},
        {'kind':'input','call_index':1,'held':0,'pressed':0,'frame_before':0,'frame_after':0},
        {'kind':'input','call_index':49968,'held':0,'pressed':0,'frame_before':48739,'frame_after':48739},
        {'kind':'final','total_calls':49970,'frame':48739,'won':1,'lost':0},
        {'kind':'strict-verdict','ok':1},
    ]


class ParserTests(unittest.TestCase):
    def parse(self, data, stop=1000):
        with tempfile.TemporaryDirectory() as d:
            p=Path(d)/'trace.jsonl';p.write_text(''.join(json.dumps(x)+'\n' for x in data))
            return replay.plan(p,stop)

    def test_valid(self):
        data=rows();data[2]['held']=16;data[2]['pressed']=16
        self.assertEqual(self.parse(data),[(0,1),(1,16)])
        self.assertEqual(self.parse(data,49968),[(0,1),(1,16)])

    def test_invalid_inputs(self):
        base=rows();base[2]['held']=16;base[2]['pressed']=16
        for change in ({'pressed':0},{'held':1024},{'held':48,'pressed':48},
                       {'call_index':0},{'held':1,'pressed':0}):
            with self.subTest(change=change):
                data=copy.deepcopy(base);data[2].update(change)
                with self.assertRaises(ValueError):self.parse(data)
        data=copy.deepcopy(base);data[3]['call_index']=49969
        with self.assertRaises(ValueError):self.parse(data)
        data=copy.deepcopy(base);data.insert(4,dict(data[3]))
        with self.assertRaises(ValueError):self.parse(data)

    def test_bound_and_verdict(self):
        data=rows();data[2]['held']=16;data[2]['pressed']=16
        with self.assertRaises(ValueError):self.parse(data,999)
        data.pop()
        with self.assertRaises(ValueError):self.parse(data)

if __name__=='__main__':unittest.main()
