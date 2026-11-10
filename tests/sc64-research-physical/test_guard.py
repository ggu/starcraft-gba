# SPDX-License-Identifier: GPL-3.0-or-later
import hashlib
import fcntl
import os
import signal
import time
import importlib.util
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools'))
spec=importlib.util.spec_from_file_location('research_physical',ROOT/'tools/sc64_research_physical.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)

class GuardTests(unittest.TestCase):
    def test_input_pin_disk_and_output_checks(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory)
            class Args: pass
            a=Args()
            for name in ('rom','elf','runner','tape','resume_tape'):
                path=root/name;path.write_bytes(name.encode());setattr(a,name,path)
            a.output=root/'not-created'/'output'
            runner_hash=hashlib.sha256(b'runner').hexdigest()
            with patch.object(m,'RUNNER_SHA',runner_hash):
                self.assertEqual(len(m.validate_inputs(a)),5)
                a.runner.write_bytes(b'untrusted')
                with self.assertRaises(ValueError):m.validate_inputs(a)
                a.runner.write_bytes(b'runner')
                with patch.object(m.shutil,'disk_usage',return_value=type('D',(),{'free':m.RESERVE-1})()):
                    with self.assertRaises(ValueError):m.validate_inputs(a)
                a.output.mkdir(parents=True)
                with self.assertRaises(ValueError):m.validate_inputs(a)

    def test_output_limit_counts_nested_files(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory);(root/'nested').mkdir();(root/'nested'/'log').write_bytes(b'x'*11)
            with patch.object(m,'OUTPUT_LIMIT',10):
                self.assertGreater(m.output_bytes(root),m.OUTPUT_LIMIT)

    def test_internal_worker_option_does_not_exist(self):
        with self.assertRaises(SystemExit):
            m.main(['--internal-worker','--rom','r','--elf','e','--runner','x','--tape','t','--resume-tape','u','--output','o'])

    def test_bounded_wait_and_hung_descendant_cleanup(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory)
            class Args: pass
            a=Args()
            for name in ('rom','elf','runner','tape','resume_tape'):
                path=root/name;path.write_bytes(name.encode());setattr(a,name,path)
            a.output=root/'output'
            lock_path=str(root/'lock')
            runner_hash=hashlib.sha256(b'runner').hexdigest()
            with patch.object(m,'LOCK_PATH',lock_path),patch.object(m,'RUNNER_SHA',runner_hash), \
                 patch.object(m,'WALL_SECONDS',0.6),patch.object(m,'CLEANUP_SECONDS',0.2), \
                 patch.object(m,'POLL_SECONDS',0.02):
                with open(lock_path,'a') as held:
                    fcntl.flock(held,fcntl.LOCK_EX|fcntl.LOCK_NB)
                    start=time.monotonic();self.assertEqual(m.supervise(a),124)
                    self.assertLess(time.monotonic()-start,1.5)
                    self.assertFalse(a.output.exists())
                readfd,writefd=os.pipe()
                def hung(_a):
                    os.close(readfd)
                    if os.fork()==0:
                        while True:time.sleep(10)
                    while True:time.sleep(10)
                try:
                    start=time.monotonic();self.assertEqual(m.supervise(a,hung),124)
                    self.assertLess(time.monotonic()-start,1.5)
                    os.close(writefd);writefd=-1
                    import select
                    self.assertTrue(select.select([readfd],[],[],1)[0])
                    self.assertEqual(os.read(readfd,1),b'')
                finally:
                    os.close(readfd)
                    if writefd>=0:os.close(writefd)

    def test_immediate_parent_term_cleans_owned_child(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory)
            class Args: pass
            a=Args()
            for name in ('rom','elf','runner','tape','resume_tape'):
                path=root/name;path.write_bytes(name.encode());setattr(a,name,path)
            a.output=root/'output'
            readfd,writefd=os.pipe(); real_fork=os.fork
            def fork_and_term():
                pid=real_fork()
                if pid: os.kill(os.getpid(),signal.SIGTERM)
                return pid
            def hung(_a):
                os.close(readfd)
                while True:time.sleep(10)
            try:
                with patch.object(m,'LOCK_PATH',str(root/'lock')), \
                     patch.object(m,'RUNNER_SHA',hashlib.sha256(b'runner').hexdigest()), \
                     patch.object(m.os,'fork',side_effect=fork_and_term), \
                     patch.object(m,'WALL_SECONDS',1),patch.object(m,'CLEANUP_SECONDS',0.2), \
                     patch.object(m,'POLL_SECONDS',0.02):
                    with self.assertRaises(SystemExit) as caught:m.supervise(a,hung)
                    self.assertEqual(caught.exception.code,128+signal.SIGTERM)
                os.close(writefd);writefd=-1
                import select
                self.assertTrue(select.select([readfd],[],[],1)[0])
                self.assertEqual(os.read(readfd,1),b'')
            finally:
                os.close(readfd)
                if writefd>=0:os.close(writefd)

if __name__=='__main__':unittest.main()
