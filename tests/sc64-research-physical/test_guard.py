# SPDX-License-Identifier: GPL-3.0-or-later
import hashlib
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

    def test_worker_cannot_bypass_lock(self):
        with patch.dict(m.os.environ,{'SC64_RESEARCH_PROBE_LOCK_FD':'-1'}):
            with self.assertRaises(SystemExit):
                m.main(['--internal-worker','--rom','r','--elf','e','--runner','x','--tape','t','--resume-tape','u','--output','o'])

if __name__=='__main__':unittest.main()
