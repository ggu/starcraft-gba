# SPDX-License-Identifier: GPL-3.0-or-later
"""The walk output must retain its own generator when the bank is absent."""
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]

class WalkMakeTest(unittest.TestCase):
    def test_missing_walk_uses_walk_generator(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'Makefile').write_bytes((ROOT / 'Makefile').read_bytes())
            (root / 'tools').symlink_to(ROOT / 'tools', target_is_directory=True)
            facing = '.local/sc64/facing/generated/sc64_unit_facing_generated.h'
            target = '.local/sc64/walk/generated/sc64_unit_walk_generated.h'
            result = subprocess.run(['make', '-n', '-o', facing, target], cwd=root,
                                    text=True, capture_output=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(result.stdout.count('tools/sc64_unit_walk_frames.py'), 1)
            self.assertNotIn('tools/sc64_unit_facing.py', result.stdout)
            self.assertNotIn('sc64_unit_walk_generated.h', result.stderr)

if __name__ == '__main__':
    unittest.main()
