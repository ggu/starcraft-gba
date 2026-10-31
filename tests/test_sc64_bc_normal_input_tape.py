# SPDX-License-Identifier: GPL-3.0-or-later
import copy
import json
import pathlib
import subprocess
import sys
import tempfile
import unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
import sc64_bc_normal_input_tape as tape

def witness():
    return {'profile':'unified-race-skirmish','events':[
        {'kind':'input','host_tick':1,'frame':0,'held':128,'pressed':128},
        {'kind':'input','host_tick':2,'frame':0,'held':0,'pressed':0},
        {'kind':'start','host_tick':20,'frame':1,'map':1,'human_race':0,'bots':[{'owner':1,'race':1}]},
        {'kind':'result','result':'impact','host_tick':17937,'frame':17658,'won':0,'lost':0,'saved_flight':1}]}

class TapeTest(unittest.TestCase):
    def test_valid_witness(self):
        self.assertEqual(tape.convert(witness()),'1 128 128\n2 0 0\n')
    def test_reject_corruption(self):
        cases=[]
        for event,key,value in [(0,'held',1024),(0,'pressed',0),(1,'host_tick',1),(1,'frame',-1),
                                (2,'map',0),(2,'human_race',False),(3,'won',True),
                                (3,'result','flight'),(3,'host_tick',17936),(3,'saved_flight',0)]:
            x=witness();x['events'][event][key]=value;cases.append(x)
        x=witness();x['events'].pop();cases.append(x)
        x=witness();x['events'].insert(3,{'kind':'mystery','host_tick':21,'frame':2});cases.append(x)
        x=witness();x['events'].insert(2,copy.deepcopy(x['events'][1]));cases.append(x)
        for item in cases:
            with self.subTest(item=item),self.assertRaises(ValueError):tape.convert(item)
    def test_cli_rejects_duplicate_keys_and_preserves_existing_output(self):
        with tempfile.TemporaryDirectory() as d:
            path=pathlib.Path(d);inp=path/'in.json';out=path/'out.txt';out.write_text('keep')
            inp.write_text(json.dumps(witness()));cmd=[sys.executable,str(ROOT/'tools/sc64_bc_normal_input_tape.py'),'--input',str(inp),'--output',str(out)]
            self.assertNotEqual(subprocess.run(cmd,capture_output=True).returncode,0);self.assertEqual(out.read_text(),'keep')
            out.unlink();inp.write_text('{"profile":"unified-race-skirmish","profile":"unified-race-skirmish","events":[]}')
            self.assertNotEqual(subprocess.run(cmd,capture_output=True).returncode,0);self.assertFalse(out.exists())
    def test_cli_rejects_source_output_before_write(self):
        out=ROOT/'tests'/'forbidden-bc-tape.txt';self.assertFalse(out.exists())
        result=subprocess.run([sys.executable,str(ROOT/'tools/sc64_bc_normal_input_tape.py'),'--input','missing','--output',str(out)],capture_output=True)
        self.assertNotEqual(result.returncode,0);self.assertIn(b'source tree',result.stderr);self.assertFalse(out.exists())
if __name__=='__main__':unittest.main()
