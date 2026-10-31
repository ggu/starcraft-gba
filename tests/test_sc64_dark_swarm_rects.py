# SPDX-License-Identifier: GPL-3.0-or-later
import tempfile
import unittest
from pathlib import Path
from tools import sc64_dark_swarm_rects as rects


class SourceRectGeneratorTest(unittest.TestCase):
    def test_rejects_unauthenticated_data_without_writing_header(self):
        with tempfile.TemporaryDirectory() as directory:
            base = Path(directory)
            source = base / 'wrong.dat'
            source.write_bytes(b'not the authenticated DAT')
            out = base / 'out'
            with self.assertRaisesRegex(ValueError, 'does not match'):
                rects.generate(source, out)
            self.assertFalse(out.exists())

    def test_rejects_output_in_tracked_tree_before_reading_input(self):
        with self.assertRaisesRegex(ValueError, 'must be under .local'):
            rects.generate(Path('/nonexistent-input'), rects.ROOT / 'tests' / 'bad-output')
