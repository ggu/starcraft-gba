#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Asset-free bounds checks for the Z8 permission accessor."""
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class Z8PermissionsTest(unittest.TestCase):
    def test_accessor_bounds_and_bits(self):
        clang = shutil.which('clang')
        if not clang:
            self.skipTest('clang is not available')
        with tempfile.TemporaryDirectory() as tmp:
            binary = Path(tmp) / 'z8-permissions'
            subprocess.run([clang, '-std=c11', '-O1', '-g', '-Wall', '-Wextra',
                            '-Werror', '-pedantic', '-fsanitize=address,undefined',
                            '-fno-sanitize-recover=all',
                            str(ROOT / 'tests/sc64_z8_permissions.c'),
                            '-o', str(binary)], check=True, capture_output=True)
            subprocess.run([str(binary)], check=True, capture_output=True, timeout=30)


if __name__ == '__main__':
    unittest.main()
