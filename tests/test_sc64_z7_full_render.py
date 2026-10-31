"""Asset-free contracts for the unpromoted Z7 presentation profile.

Copyright (C) 2026 blizzard-gba contributors
SPDX-License-Identifier: GPL-3.0-or-later
"""
import subprocess
import unittest
from pathlib import Path

from tools import verify_roms

ROOT = Path(__file__).resolve().parents[1]


class Z7FullRenderTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        snippet = ('.PHONY: print-z7-full-render\n'
                   'print-z7-full-render:\n'
                   "\t@printf '%s\\n' $(Z7_FULL_RENDER_FLAGS)\n")
        result = subprocess.run(
            ['make', '-s', '-f', 'Makefile', '-f', '-', 'print-z7-full-render'],
            cwd=ROOT, input=snippet, text=True, check=True,
            stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        cls.flags = result.stdout.splitlines()

    def test_keeps_complete_gameplay_and_optimized_queries(self):
        for flag in (
            '-DSC64_Z7_RUNTIME', '-DSC64_Z7_CAMPAIGN_ENTRY',
            '-DSC64_Z6_CAMPAIGN_ENTRY', '-DSC64_Z7_COLONY_DEFENSE_ENABLED',
            '-DSC64_CONSTRUCTION_HP', '-DSC64_DIRECT_ATTACK',
            '-DSC64_Z7_EGG_PRODUCTION', '-DSC64_Z7_SOURCE_SIGHT',
            '-DSC64_Z7_SCOURGE_COMBAT', '-DSC64_NEAREST_RESOURCE_DEPOT',
            '-DSC64_Z7_PREVALIDATED_TICK', '-DSC64_Z7_COUNT_SNAPSHOT',
            '-DSC64_Z7_SIGHT_SPANS', '-DSC64_ZP_DEFENSE_TARGETS',
            '-DSC64_ROUTE_FLOOD_SPLIT',
        ):
            with self.subTest(flag=flag):
                self.assertIn(flag, self.flags)

    def test_uses_complete_canonical_presentation_and_packed_bank(self):
        for flag in (
            '-DGBA_SPRITE_WORDS', '-DGBA_SCENE_CLIP', '-DSC64_HUD_BUFFER_REUSE',
            '-DSC64_TERRAIN_SCROLL', '-DGBA_WINDOW_BANDS',
            '-DSC64_STATIC_DIRTY_ROWS', '-DSC64_UNIT_WORD_SPRITES',
            '-DSC64_MINIMAP_BLIT', '-DSC64_UNIT_FACING',
            '-DSC64_TANK_SOURCE_VFX', '-DSC64_WEAPON_SOURCE_VFX',
            '-DSC64_UNIT_ATTACK_POSES_ENABLED', '-DSC64_DEATH_SOURCE_ART',
        ):
            with self.subTest(flag=flag):
                self.assertIn(flag, self.flags)
        self.assertIn('-I.local/sc64/zerg-z6/generated', self.flags)
        self.assertNotIn('-I.local/sc64/z6/generated', self.flags)
        self.assertLess(self.flags.index('-I.local/sc64/zerg-z6/generated'),
                        self.flags.index('-I.local/sc64/z5/generated'))

    def test_distinct_probe_identity_and_zerg_profiles(self):
        self.assertEqual(verify_roms.TARGETS['--zerg-z7-full-render'], (
            'StarCraft-Zerg-Z7-Full-Render-Probe-GBA',
            'starcraft-zerg-z7-full-render-probe', 'SCZR', 'SC Z7 FULL36'))
        self.assertEqual(verify_roms.TARGETS['--zerg'], (
            'StarCraft-Zerg-Z7-GBA', 'starcraft-zerg-z7', 'SCZ7', 'SC ZERG Z7'))
        self.assertEqual(verify_roms.TARGETS['--zerg-z6'], (
            'StarCraft-Zerg-Z6-GBA', 'starcraft-zerg-z6', 'SCZ6', 'SC ZERG Z6'))


if __name__ == '__main__':
    unittest.main()
