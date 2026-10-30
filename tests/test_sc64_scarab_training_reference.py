# SPDX-License-Identifier: GPL-3.0-or-later
"""Compile the isolated Scarab scheduler reference under sanitizers."""
import pathlib
import subprocess
import tempfile
import unittest
ROOT = pathlib.Path(__file__).resolve().parents[1]
class ScarabTrainingReferenceTest(unittest.TestCase):
    def test_source_tick_reference(self):
        with tempfile.TemporaryDirectory() as d:
            binary = pathlib.Path(d) / 'test'
            subprocess.run(['clang','-std=c11','-O1','-g','-Wall','-Wextra','-Werror',
                            '-fsanitize=address,undefined',
                            str(ROOT/'tests/sc64_scarab_training_reference.c'),
                            '-o',str(binary)],check=True)
            subprocess.run([str(binary)],check=True)
if __name__ == '__main__': unittest.main()
