# SPDX-License-Identifier: GPL-3.0-or-later
"""Validate the physical probe's lock, bounds and owned-child cleanup."""
import hashlib
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest import mock
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import sc64_early_protoss_save_probe as probe

class ProbeSafety(unittest.TestCase):
 def setUp(self):
  self.tmp=tempfile.TemporaryDirectory();self.addCleanup(self.tmp.cleanup)
  self.root=Path(self.tmp.name);self.lock=self.root/'shared.lock';self.files=[]
  for n in ('rom','elf','runner'):
   p=self.root/n;p.write_bytes(n.encode());self.files.append(p)
  self.output=self.root/'new'/'result'
  self.args=sum(([f'--{n}',str(p)] for n,p in zip(('rom','elf','runner'),self.files)),[])+['--output',str(self.output)]
  self.patches=[mock.patch.object(probe,'LOCK_PATH',str(self.lock)),
   *[mock.patch.object(probe,n,hashlib.sha256(p.read_bytes()).hexdigest()) for n,p in zip(('ROM_SHA','ELF_SHA','RUNNER_SHA'),self.files)]]
  for p in self.patches:p.start();self.addCleanup(p.stop)
 def test_bad_counts_and_internal_worker_refused(self):
  with mock.patch.object(probe.subprocess,'Popen') as start, mock.patch.dict(os.environ,{},clear=True):
   with self.assertRaises(SystemExit):probe.main(self.args+['--play','0'])
   with self.assertRaises(SystemExit):probe.main(self.args+['--internal-worker'])
   start.assert_not_called();self.assertFalse(self.output.exists())
 def test_disk_and_existing_output_fail_before_child(self):
  with mock.patch.object(probe.subprocess,'Popen') as start:
   with mock.patch.object(probe.shutil,'disk_usage',return_value=mock.Mock(free=1024)):
    with self.assertRaises(SystemExit):probe.main(self.args)
   self.output.mkdir(parents=True)
   with self.assertRaises(SystemExit):probe.main(self.args)
   start.assert_not_called()
 def test_hash_is_rechecked_before_child(self):
  self.files[0].write_bytes(b'changed')
  with mock.patch.object(probe.subprocess,'Popen') as start:
   with self.assertRaises(SystemExit):probe.main(self.args)
   start.assert_not_called();self.assertFalse(self.output.exists())
 def test_input_changed_while_waiting_is_rejected(self):
  def changed(_fd,_mode): self.files[0].write_bytes(b'changed-after-wait')
  with mock.patch.object(probe.fcntl,'flock',side_effect=changed), mock.patch.object(probe.subprocess,'Popen') as start:
   with self.assertRaises(SystemExit):probe.main(self.args)
   start.assert_not_called();self.assertFalse(self.output.exists())
 def test_worker_requires_the_actual_inherited_lock(self):
  with open(self.lock,'w') as lock, mock.patch.object(probe,'run',return_value=7) as run:
   probe.fcntl.flock(lock,probe.fcntl.LOCK_EX)
   with mock.patch.dict(os.environ,{'SC64_EARLY_PROBE_LOCK_FD':str(lock.fileno())}):
    self.assertEqual(probe.main(self.args+['--internal-worker']),7)
   run.assert_called_once()
 def test_timeout_stops_only_owned_child_group(self):
  proc=mock.Mock(pid=1234);proc.poll.return_value=-15;proc.wait.side_effect=[subprocess.TimeoutExpired('child',240),-15]
  with mock.patch.object(probe.subprocess,'Popen',return_value=proc) as start, mock.patch.object(probe.os,'killpg') as kill:
   self.assertEqual(probe.main(self.args),124)
   self.assertTrue(start.call_args.kwargs['start_new_session']);self.assertTrue(start.call_args.kwargs['pass_fds'])
   self.assertEqual(kill.call_args_list,[mock.call(1234,probe.signal.SIGTERM),mock.call(1234,probe.signal.SIGKILL)]);self.assertFalse(self.output.exists())
 def test_interrupted_wait_cleans_owned_group(self):
  proc=mock.Mock(pid=2345);proc.poll.return_value=-15;proc.wait.side_effect=[KeyboardInterrupt(),-15]
  with mock.patch.object(probe.subprocess,'Popen',return_value=proc), mock.patch.object(probe.os,'killpg') as kill:
   with self.assertRaises(KeyboardInterrupt):probe.main(self.args)
   self.assertEqual(kill.call_args_list,[mock.call(2345,probe.signal.SIGTERM),mock.call(2345,probe.signal.SIGKILL)])
 def test_terminated_wait_cleans_owned_group(self):
  proc=mock.Mock(pid=3456);proc.poll.return_value=-15;proc.wait.side_effect=[SystemExit(143),-15]
  with mock.patch.object(probe.subprocess,'Popen',return_value=proc), mock.patch.object(probe.os,'killpg') as kill:
   with self.assertRaises(SystemExit):probe.main(self.args)
   self.assertEqual(kill.call_args_list,[mock.call(3456,probe.signal.SIGTERM),mock.call(3456,probe.signal.SIGKILL)])
if __name__=='__main__':unittest.main()
