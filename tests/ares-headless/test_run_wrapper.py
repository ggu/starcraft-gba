#!/usr/bin/env python3
"""Asset-free lifecycle checks for the noninteractive callback wrapper."""
import hashlib
import os
from pathlib import Path
import runpy
import shutil
import subprocess
import sys
import tempfile
import unittest
from unittest import mock

SCRIPT = Path(__file__).resolve().parents[2] / 'experiments/ares-headless/run.py'
ROM_HASH = 'e7d21880c13953b45b8f8db6b2b6cfa02daa29d35b23799a967e96f21001b020'
VK_HASH = '432e8880bc83e068f26e47928d01cb34c501694769e8527f34118c93946e6f99'
REV = '0aafd85789215e84e1e43415c07d4c88461b7899'

class WrapperTest(unittest.TestCase):
 def exercise(self, mode):
  with tempfile.TemporaryDirectory() as temp:
   root=Path(temp); source=root/'source'; source.mkdir(); output=root/'out'
   for name in ('ares/ares/resource/resource.cpp','ares/ares/resource/resource.hpp','mia/resource/resource.cpp','mia/resource/resource.hpp'):
    p=source/name; p.parent.mkdir(parents=True,exist_ok=True); p.write_bytes(b'resource')
   rom=root/'rom'; rom.write_bytes(b'rom'); vk=root/'vk'; vk.write_bytes(b'vk'); binary=root/'binary'; binary.write_bytes(b'binary')
   original_sha=hashlib.sha256; original_resolve=Path.resolve; state={'locked':False,'ran':False,'dirty':False}
   def sha(data=b''):
    if data==b'rom': return mock.Mock(hexdigest=lambda: 'changed' if state['locked'] and mode=='rom' else ROM_HASH)
    if data==b'vk': return mock.Mock(hexdigest=lambda: 'changed' if state['locked'] and mode=='vk' else VK_HASH)
    return original_sha(data)
   def resolve(path, *args, **kwargs):
    return Path('/Applications/ares.app/Contents/Frameworks/libMoltenVK.dylib') if path==vk else original_resolve(path,*args,**kwargs)
   def git(args, **kwargs):
    return (REV+'\n') if 'rev-parse' in args else ('modified\n' if state['dirty'] else '')
   def lock(fd, operation):
    state['locked']=True
    if mode=='output': output.mkdir()
    if mode=='dirty': state['dirty']=True
   def run(args, **kwargs):
    self.assertTrue(state['locked']); state['ran']=True
    (output/'frame-000.argb').write_bytes(b'frame')
    return subprocess.CompletedProcess(args,0)
   free=0 if mode=='disk' else 10*1024**3
   usage=shutil._ntuple_diskusage(20*1024**3,0,free)
   argv=[str(SCRIPT),'--binary',str(binary),'--source',str(source),'--moltenvk',str(vk),'--rom',str(rom),'--output',str(output),'--scenario','neutral']
   with mock.patch.object(sys,'argv',argv), mock.patch('hashlib.sha256',side_effect=sha), mock.patch.object(Path,'resolve',resolve), mock.patch('subprocess.check_output',side_effect=git), mock.patch('subprocess.run',side_effect=run), mock.patch('fcntl.flock',side_effect=lock), mock.patch('shutil.disk_usage',return_value=usage):
    with self.assertRaises(SystemExit) as result: runpy.run_path(str(SCRIPT),run_name='__main__')
   if mode=='ok':
    self.assertEqual(result.exception.code,0); self.assertTrue(state['ran']); self.assertTrue((output/'manifest.json').is_file())
   else:
    self.assertFalse(state['ran']); self.assertFalse((output/'input').exists())
    self.assertIn({'disk':'disk reserve','dirty':'source checkout is dirty','output':'output must be new','rom':'wrong owned ROM','vk':'wrong MoltenVK binary'}[mode],str(result.exception))
 def test_success_under_lock(self): self.exercise('ok')
 def test_rechecks_queued_source(self): self.exercise('dirty')
 def test_rechecks_queued_output(self): self.exercise('output')
 def test_rechecks_queued_rom(self): self.exercise('rom')
 def test_rechecks_queued_vk(self): self.exercise('vk')
 def test_disk_reserve_before_mutation(self): self.exercise('disk')

if __name__=='__main__': unittest.main()
