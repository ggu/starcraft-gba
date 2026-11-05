# SPDX-License-Identifier: GPL-3.0-or-later
import tempfile
import unittest
from pathlib import Path
from tools import sc64_guardian_cocoon_assets as assets

class CocoonAssetBoundaryTest(unittest.TestCase):
    def test_refuses_output_outside_private_worktree(self):
        with tempfile.TemporaryDirectory() as tmp:
            with self.assertRaisesRegex(ValueError, 'private output'):
                assets.prepare(Path('unused-rom'), Path('unused-extractor'), Path(tmp) / 'output')

    def test_refuses_unpinned_extractor_before_reading_rom(self):
        with tempfile.TemporaryDirectory(dir=assets.ROOT / '.local') as tmp:
            extractor = Path(tmp) / 'extractor.py'
            extractor.write_text('not the pinned extractor')
            with self.assertRaisesRegex(ValueError, 'pinned source'):
                assets.prepare(Path('unused-rom'), extractor, Path(tmp) / 'output')
            self.assertFalse((Path(tmp) / 'output').exists())

if __name__ == '__main__':
    unittest.main()
