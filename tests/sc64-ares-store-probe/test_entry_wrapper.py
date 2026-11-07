# SPDX-License-Identifier: GPL-3.0-or-later
import struct, csv, hashlib, importlib.util, tempfile, unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('hybrid_snapshot',ROOT/'experiments/ares-headless/hybrid_entry_events.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)

class SnapshotWrapperTest(unittest.TestCase):
 def test_pinned_plan(self):
  data=(ROOT/'tests/sc64-ares-observer/fixtures/hybrid-probe-attempt.txt').read_bytes()
  self.assertEqual(hashlib.sha256(data).hexdigest(),m.COMMAND_SHA)
  rows=m.planned_rows(data)
  self.assertEqual(tuple(r[0]+r[1] for r in rows if r[-1]==3),m.EXPECTED_RAM_STEPS)
  with self.assertRaises(ValueError):m.planned_rows(data.replace(b'1 0 0 0 3\n',b'1 0 0 0 0\n',1))
 def test_witness_requires_exact_snapshots_and_status(self):
  data=(ROOT/'tests/sc64-ares-observer/fixtures/hybrid-probe-attempt.txt').read_bytes(); rows=m.planned_rows(data)
  with tempfile.TemporaryDirectory() as d:
   p=Path(d)
   (p/'commands.csv').write_text('begin,steps,mask,x,y,capture\n'+''.join(','.join(map(str,r))+'\n' for r in rows))
   (p/'frames.csv').write_text('callback,observed_step,width,height,bytes\n0,4258,640,240,614400\n1,4320,640,240,614400\n')
   for i in range(2):
    with (p/f'live-{i:03}.argb').open('wb') as f:f.truncate(614400)
   (p/'ram.csv').write_text(''.join(f'{i},{step},{8*1024**2}\n' for i,step in enumerate(m.EXPECTED_RAM_STEPS)))
   for i in range(3):
    with (p/f'ram-{i:03}.bin').open('wb') as f:f.truncate(8*1024**2)
   (p/'status.log').write_text('frontend=hybrid_snapshot_entries jit_before_cpu=1 jit_before_rsp=1 homebrew=0 hybrid_transition=1 exact_view_equal=1 cpu_after=0 rsp_after=0\ncompleted_steps=4322 gpu_ready=1 ')
   event=struct.pack('<QQQQIIIHH',0x80001234,0x802ffd10,1,0x802ffd10,0xac830004,0x2ffd10,4202,4,4)
   (p/'stores.bin').write_bytes(event)
   with (p/'status.log').open('a') as f:f.write('\nstore_baseline_verified=1 store_count=1 store_overflow=0\n')
   entry=struct.pack('<QQQQQQII',0xffffffff8000c390,0xffffffff8013b418,1,2,3,4,0x27bdffe0,4202)
   (p/'entries.bin').write_bytes(entry)
   with (p/'status.log').open('a') as f:f.write('entry_count=1 entry_overflow=0\n')
   self.assertTrue(m.planned_witness(p,rows))
   (p/'entries.bin').write_bytes(b'');self.assertFalse(m.planned_witness(p,rows))
   (p/'entries.bin').write_bytes(entry)
   for bad in (b'',b'x',struct.pack('<QQQQIIIHH',0,0,256,0,0,0x2ffd10,4202,1,0)):
    (p/'stores.bin').write_bytes(bad);self.assertFalse(m.planned_witness(p,rows))
   (p/'stores.bin').write_bytes(event)
   good=(p/'frames.csv').read_text()
   for old,new in [('640','639'),('240','239'),('614400','614399')]:
    (p/'frames.csv').write_text(good.replace(old,new,1));self.assertFalse(m.planned_witness(p,rows))
   (p/'frames.csv').write_text(good)
   with (p/'live-001.argb').open('wb') as f:f.truncate(614399)
   self.assertFalse(m.planned_witness(p,rows))
   with (p/'live-001.argb').open('wb') as f:f.truncate(614400)
   (p/'ram-002.bin').unlink();self.assertFalse(m.planned_witness(p,rows))
   with (p/'ram-002.bin').open('wb') as f:f.truncate(8*1024**2)
   (p/'ram.csv').write_text('0,4202,8388608\n');self.assertFalse(m.planned_witness(p,rows))
   (p/'ram.csv').write_text(''.join(f'{i},{step},{8*1024**2}\n' for i,step in enumerate(m.EXPECTED_RAM_STEPS)))
   (p/'status.log').write_text('frontend=hybrid_snapshot cpu_after=1 gpu_ready=1 ');self.assertFalse(m.planned_witness(p,rows))
 def test_unqualified_binary_is_denied(self):
  with tempfile.TemporaryDirectory() as d:
   p=Path(d)/'binary';p.write_bytes(b'fake')
   with self.assertRaises(ValueError):m.require_binary(p)

if __name__=='__main__':unittest.main()
