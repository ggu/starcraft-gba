# SPDX-License-Identifier: GPL-3.0-or-later
"""The preview verifier preserves both standard RAM reserve boundaries."""
import sys
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import verify_sc64_egg_preview as preview

class PreviewRamTest(unittest.TestCase):
    def test_valid_and_boundary_rejection(self):
        self.assertEqual(preview.ram_ends('0203dfff B __bss_end\n03004fff T __iwram_end\n'),
                         {'__bss_end': 0x0203dfff, '__iwram_end': 0x03004fff})
        for symbols in ('0203e000 B __bss_end\n03004fff T __iwram_end\n',
                        '0203dfff B __bss_end\n03005000 T __iwram_end\n',
                        '0203dfff B __bss_end\n',
                        '0203dfff B __bss_end\n0203dfff B __bss_end\n03004fff T __iwram_end\n'):
            with self.subTest(symbols=symbols), self.assertRaises(ValueError):
                preview.ram_ends(symbols)
