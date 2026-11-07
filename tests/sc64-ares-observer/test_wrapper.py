# SPDX-License-Identifier: GPL-3.0-or-later
import hashlib, importlib.util, os, signal, subprocess, sys, tempfile, unittest
from pathlib import Path
from unittest.mock import MagicMock, patch
ROOT=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('actor_observer',ROOT/'experiments/ares-headless/actor_observer.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)

class WrapperTest(unittest.TestCase):
 def test_limit_modes_are_fixed(self):
  self.assertEqual(m.limits_for_mode('strict'),(2*1024**2,8*1024**2))
  self.assertEqual(m.limits_for_mode('metal-compatible'),(8*1024**2,8*1024**2))
  with self.assertRaises(ValueError):m.limits_for_mode('unbounded')

 def test_binary_admission_and_changed_input(self):
  with tempfile.TemporaryDirectory() as d:
   p=Path(d)/'probe';p.write_bytes(b'qualified')
   with patch.object(m,'BINARY_SHA',hashlib.sha256(b'qualified').hexdigest()):
    m.require_binary(p);p.write_bytes(b'changed')
    with self.assertRaises(ValueError):m.require_binary(p)
 def test_binary_rechecked_after_lock_before_output(self):
  with tempfile.TemporaryDirectory() as d:
   root=Path(d);rom=root/'rom';vk=root/'vk';binary=root/'probe';out=root/'out'
   rom.write_bytes(b'rom');vk.write_bytes(b'vk');binary.write_bytes(b'qualified')
   def changed(*_args):binary.write_bytes(b'changed')
   args=['--rom',str(rom),'--moltenvk',str(vk),'--binary',str(binary),'--source',str(root),'--output',str(out)]
   with patch.object(m,'ROM_SHA',hashlib.sha256(b'rom').hexdigest()), \
        patch.object(m,'VK_SHA',hashlib.sha256(b'vk').hexdigest()), \
        patch.object(m,'VK_PATH',vk.resolve()),patch.object(m,'BINARY_SHA',hashlib.sha256(b'qualified').hexdigest()), \
        patch.object(m,'LOCK',str(root/'lock')),patch.object(m,'clean',return_value=True), \
        patch.object(m.fcntl,'flock',side_effect=changed):
    with self.assertRaises(ValueError):m.main(args)
   self.assertFalse(out.exists())
 def test_output_volume_and_low_reserve(self):
  with tempfile.TemporaryDirectory() as d:
   target=Path(d)/'missing'/'out'; seen=[]
   def usage(path):
    seen.append(path);return type('D',(),{'free':5*1024**3-1})()
   with patch.object(m.shutil,'disk_usage',side_effect=usage):
    with self.assertRaises(ValueError):m.check_reserve(target)
   self.assertEqual(seen,[Path(d)])
 def test_already_exited_child_is_not_signalled(self):
  child=MagicMock();child.poll.return_value=0
  child.stdin=child.stdout=child.stderr=None
  m.terminate_child(child)
  child.terminate.assert_not_called();child.kill.assert_not_called();child.wait.assert_not_called()
 def test_cleanup_on_selector_error_and_signal(self):
  for mode in ('error','interrupt','term'):
   with self.subTest(mode=mode), tempfile.TemporaryDirectory() as d:
    child=[];real_popen=subprocess.Popen
    def spawn(*a,**kw):
     p=real_popen(*a,**kw);child.append(p);return p
    class FailingSelector:
     def register(self,*_args):pass
     def close(self):pass
     def select(self,*_args):
      if mode=='error':raise OSError('injected selector failure')
      if mode=='interrupt':raise KeyboardInterrupt()
      os.kill(os.getpid(),signal.SIGTERM)
    with patch.object(m.subprocess,'Popen',side_effect=spawn), tempfile.TemporaryFile() as log, tempfile.TemporaryFile() as err:
     expected=OSError if mode=='error' else KeyboardInterrupt if mode=='interrupt' else SystemExit
     with self.assertRaises(expected):
      m.relay([sys.executable,'-c','import time; time.sleep(60)'],d,os.environ.copy(),log,err,{},FailingSelector)
    self.assertEqual(len(child),1);self.assertIsNotNone(child[0].poll())
    with self.assertRaises(ProcessLookupError):os.kill(child[0].pid,0)

if __name__=='__main__':unittest.main()
