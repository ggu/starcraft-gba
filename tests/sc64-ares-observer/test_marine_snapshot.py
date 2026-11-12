# SPDX-License-Identifier: GPL-3.0-or-later
import hashlib,importlib.util,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('marine',ROOT/'experiments/ares-headless/marine_snapshot.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
class MarineSnapshotTest(unittest.TestCase):
 def test_no_stop_only_neutralizes_intervention(self):
  fixtures=ROOT/'tests/sc64-ares-observer/fixtures'
  stop=(fixtures/'native-marine-discovery-late.txt').read_bytes()
  control=(fixtures/'native-marine-no-stop-late.txt').read_bytes()
  _,sr=m.admit_plan(stop,False);_,cr=m.admit_plan(control,True)
  self.assertEqual([(s,c) for s,c in zip(sr,cr,strict=True) if s!=c],
                   [([7170,10,256,0,0,0],[7170,10,0,0,0,0])])
  with self.assertRaises(ValueError):m.admit_plan(control,False)
  with self.assertRaises(ValueError):m.admit_plan(stop,True)
 def test_pause_uses_only_the_native_start_intervention(self):
  fixtures=ROOT/'tests/sc64-ares-observer/fixtures'
  control=(fixtures/'native-marine-no-stop-late.txt').read_bytes()
  pause=(fixtures/'native-marine-pause-late.txt').read_bytes()
  _,cr=m.admit_plan(control,True);_,pr=m.admit_plan(pause,False,True)
  self.assertEqual([(c,p) for c,p in zip(cr,pr,strict=True) if c!=p],
                   [([7170,10,0,0,0,0],[7170,10,2,0,0,0])])
  for ns,pa in ((False,False),(True,False),(True,True)):
   with self.assertRaises(ValueError):m.admit_plan(pause,ns,pa)
  with self.assertRaises(ValueError):m.admit_plan(control,False,True)
 def test_plan_prefix_and_capture_boundaries(self):
  d=(ROOT/'tests/sc64-ares-observer/fixtures/native-marine-discovery-late.txt').read_bytes();self.assertEqual(hashlib.sha256(d).hexdigest(),m.COMMAND_SHA);rows=m.planned_rows(d)
  self.assertEqual([(r[0],r[2]) for r in rows if r[2] in (64,256)],[(7140,64),(7170,256)])
  with self.assertRaises(ValueError):m.planned_rows(d.replace(b'1 0 0 0 2\n',b'1 0 0 0 0\n',1))
 def test_witness_exact_files(self):
  d=(ROOT/'tests/sc64-ares-observer/fixtures/native-marine-discovery-late.txt').read_bytes();rows=m.planned_rows(d)
  with tempfile.TemporaryDirectory() as td:
   p=Path(td);(p/'commands.csv').write_text('begin,steps,mask,x,y,capture\n'+''.join(','.join(map(str,r))+'\n' for r in rows))
   frames='callback,observed_step,width,height,bytes\n'+''.join(f'{i},{s},640,240,614400\n' for i,s in enumerate(m.EXPECTED_FRAME_STEPS));(p/'frames.csv').write_text(frames)
   (p/'ram.csv').write_text(''.join(f'{i},{s},8388608\n' for i,s in enumerate(m.EXPECTED_RAM_STEPS)))
   for i in range(3):
    for name,size in [(f'live-{i:03}.argb',614400),(f'ram-{i:03}.bin',8388608)]:
     with (p/name).open('wb') as f:f.truncate(size)
   (p/'status.log').write_text('completed_steps=7231 frontend=hybrid_snapshot jit_before_cpu=1 jit_before_rsp=1 homebrew=0 hybrid_transition=1 exact_view_equal=1 cpu_after=0 rsp_after=0 gpu_ready=1 ')
   self.assertTrue(m.planned_witness(p,rows));(p/'live-002.argb').write_bytes(b'bad');self.assertFalse(m.planned_witness(p,rows))
   (p/'frames.csv').write_text(frames.replace('640,240','1,240',1));self.assertFalse(m.planned_witness(p,rows))
if __name__=='__main__':unittest.main()
