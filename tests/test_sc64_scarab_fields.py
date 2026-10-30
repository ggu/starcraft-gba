# SPDX-License-Identifier: GPL-3.0-or-later
"""Compile the field test with the current engine's actual compact Unit."""
import pathlib
import subprocess
import tempfile
import unittest
ROOT = pathlib.Path(__file__).resolve().parents[1]
class ScarabFieldsTest(unittest.TestCase):
    def test_actual_unit_and_codecs(self):
        game = (ROOT / 'starcraft/game.c').read_text()
        end = game.index('} Unit;') + len('} Unit;')
        start = game.rindex('typedef struct {', 0, end)
        definition = game[start:end]
        fixture = (ROOT / 'tests/sc64_scarab_fields.c').read_text()
        fixture = fixture.replace('/* SC64_REAL_UNIT_DEFINITION */', definition)
        fixture = fixture.replace('"../starcraft/', '"' + str(ROOT / 'starcraft') + '/')
        with tempfile.TemporaryDirectory() as d:
            source = pathlib.Path(d) / 'test.c'; binary = pathlib.Path(d) / 'test'
            source.write_text(fixture)
            subprocess.run(['clang', '-std=c11', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-Wno-unused-function', '-Wno-tautological-constant-out-of-range-compare', '-fsanitize=address,undefined', '-DSC64_LATE_TERRAN_STORAGE', str(source), '-o', str(binary)], check=True)
            subprocess.run([str(binary)], check=True)
if __name__ == '__main__': unittest.main()
