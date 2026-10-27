#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Asset-free host checks for the typed Z8 live-world callback bridge."""
from __future__ import annotations

import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'tests/sc64_z8_world.c'
FLAGS = ['-std=c11', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-pedantic',
         '-fsanitize=address,undefined', '-fno-sanitize-recover=all']


class Z8WorldAdapterTest(unittest.TestCase):
    def test_typed_live_world_callbacks(self) -> None:
        clang = shutil.which('clang')
        if not clang:
            self.skipTest('clang is not available')
        with tempfile.TemporaryDirectory() as tmp:
            binary = Path(tmp) / 'sc64-z8-world'
            subprocess.run([clang, *FLAGS, f'-I{ROOT}', str(SOURCE),
                            '-o', str(binary)], check=True, capture_output=True,
                            text=True)
            result = subprocess.run([str(binary)], check=True, capture_output=True,
                                    text=True, timeout=30)
        self.assertIn('sc64_z8_world: ok', result.stdout)


if __name__ == '__main__':
    unittest.main()
