# SPDX-License-Identifier: GPL-3.0-or-later
"""Check late-Zerg release vector inheritance and comparison toggles."""
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class LateZergRouteVectors(unittest.TestCase):
    def test_full_field_flags_and_no_monotone(self):
        with tempfile.TemporaryDirectory() as directory:
            fixture = Path(directory) / 'vectors.mk'
            fixture.write_text(''.join(
                f'$(info Z{n}:$(ZERG_Z{n}_RELEASE_FLAGS))\n' for n in (7, 8, 9, 10)
            ) + '.PHONY: vector-fixture\nvector-fixture:\n\t@:\n')
            for index, fresh in ((0, 0), (1, 0), (1, 1)):
                result = subprocess.run(
                    ['make', '-s', '-f', 'Makefile', '-f', str(fixture),
                     f'CANONICAL_ROUTE_INDEX={index}',
                     f'CANONICAL_ROUTE_NEW_CELL={fresh}', 'vector-fixture'],
                    cwd=ROOT, text=True, capture_output=True, check=True)
                lines = dict(line.split(':', 1) for line in result.stdout.splitlines()
                             if line.startswith(('Z7:', 'Z8:', 'Z9:', 'Z10:')))
                self.assertEqual(set(lines), {'Z7', 'Z8', 'Z9', 'Z10'})
                for name, value in lines.items():
                    flags = value.split()
                    for flag in ('-DSC64_ROUTE_WORD_CLEAR', '-DSC64_ROUTE_VISIT_GRID'):
                        self.assertEqual(flags.count(flag), index, (name, flag))
                    self.assertEqual(flags.count('-DSC64_ROUTE_NEW_CELL'), fresh, name)
                    self.assertIn('-DSC64_ROUTE_PACK3', flags, name)
                    self.assertIn('-DSC64_ROUTE_FLOOD_SPLIT', flags, name)
                    self.assertNotIn('-DSC64_ROUTE_PARTIAL_FLOOD', flags, name)
                    self.assertNotIn('-DSC64_ROUTE_MONOTONE_PROBE', flags, name)


if __name__ == '__main__':
    unittest.main()
