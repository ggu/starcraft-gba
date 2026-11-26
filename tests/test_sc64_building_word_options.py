# SPDX-License-Identifier: GPL-3.0-or-later
"""Check the shared building compositor selection without game assets."""

import os
from pathlib import Path
import subprocess
import unittest


ROOT = Path(__file__).resolve().parents[1]
BANKS = (
    'TERRAN_BUILDING_WORD_INPUTS',
    'Z10_SELECTED_BUILDING_WORD_HEADER',
    'P10_BUILDING_WORD_HEADER',
    'SHOWCASE_BUILDING_WORD_INPUTS',
)
PACKED = (
    'terran-base-building-words',
    'z10-terran-building-words',
    'p10-hatchery-overmind-nexus-command_center-barracks-supply_depot-words',
    'showcase-terran-base-building-words',
)
STOCK = ('', 'generated', 'p10-hatchery-overmind-nexus-words', '')
DEFAULT = (
    'terran-command_center-barracks-supply_depot-pylon-gateway-forge-words',
    PACKED[1],
    'p10-hatchery-overmind-nexus-command_center-barracks-supply_depot-pylon-gateway-forge-words',
    'showcase-command_center-barracks-supply_depot-pylon-gateway-forge-words',
)
PROTOSS_ONLY = (
    'terran-pylon-gateway-forge-words', STOCK[1],
    'p10-hatchery-overmind-nexus-pylon-gateway-forge-words',
    'showcase-pylon-gateway-forge-words',
)


class BuildingWordOptionsTest(unittest.TestCase):
    def plan(self, *settings):
        format_args = ' '.join(f"'$({variable})'" for variable in BANKS)
        recipe = f'.PHONY: print-building-banks\nprint-building-banks:\n\t@printf "%s\\n" {format_args}\n'
        return subprocess.run(
            ['make', '-s', '-f', 'Makefile', '-f', '-', *settings, 'print-building-banks'],
            input=recipe, cwd=ROOT, text=True, capture_output=True, check=False,
            env={'PATH': os.environ.get('PATH', os.defpath)},
        )

    def banks(self, *settings):
        plan = self.plan(*settings)
        self.assertEqual(plan.returncode, 0, plan.stderr)
        return tuple(Path(line).parent.name if line else '' for line in plan.stdout.splitlines())

    def test_normal_and_rollback_use_the_expected_banks(self):
        self.assertEqual(self.banks(), DEFAULT)
        self.assertEqual(self.banks('CANONICAL_PROTOSS_BASE_BUILDING_WORDS=0'), PACKED)
        self.assertEqual(self.banks('CANONICAL_TERRAN_BUILDING_WORDS=0'), PROTOSS_ONLY)
        self.assertEqual(self.banks('CANONICAL_TERRAN_BUILDING_WORDS=0',
                                    'CANONICAL_PROTOSS_BASE_BUILDING_WORDS=0'), STOCK)
        for option in ('CANONICAL_TERRAN_BUILDING_WORDS', 'CANONICAL_PROTOSS_BASE_BUILDING_WORDS'):
            with self.subTest(option=option):
                self.assertNotEqual(self.plan(f'{option}=2').returncode, 0)

    def test_profile_overrides_preserve_other_bank_selections(self):
        options = ('TERRAN_BUILDING_WORDS', 'Z10_TERRAN_BUILDING_WORDS',
                   'P10_TERRAN_BUILDING_WORDS', 'SHOWCASE_TERRAN_BUILDING_WORDS')
        for protoss, enabled, disabled in ((1, DEFAULT, PROTOSS_ONLY), (0, PACKED, STOCK)):
            for index, option in enumerate(options):
                with self.subTest(protoss=protoss, option=option):
                    expected = enabled[:index] + (disabled[index],) + enabled[index + 1:]
                    self.assertEqual(self.banks(f'CANONICAL_PROTOSS_BASE_BUILDING_WORDS={protoss}',
                                                f'{option}=0'), expected)

    def test_incompatible_renderer_skips_protoss_extension(self):
        self.assertEqual(self.banks('CANONICAL_PROTOSS_BASE_BUILDING_WORDS=1',
                                    'CFLAGS=-DSC64_TILE_RENDERER'), ('', 'generated', '', ''))

    def test_configuration_changes_when_the_selected_roster_changes(self):
        names = ('TERRAN_NORMAL_OPTIONS', 'P10_BUILDING_WORD_DIR', 'SHOWCASE_BUILDING_WORD_ENABLED')
        recipe = '.PHONY: print-config\nprint-config:\n\t@printf "%s\\n" ' + ' '.join(f"'$({v})'" for v in names) + '\n'
        def config(value):
            run = subprocess.run(['make', '-s', '-f', 'Makefile', '-f', '-',
                                  f'CANONICAL_PROTOSS_BASE_BUILDING_WORDS={value}', 'print-config'],
                                 input=recipe, cwd=ROOT, text=True, capture_output=True, check=True)
            return run.stdout.splitlines()
        self.assertTrue(all(a != b for a, b in zip(config(0), config(1))))

    def test_incompatible_renderers_skip_the_shared_extension(self):
        self.assertEqual(self.banks('CFLAGS=-DSC64_TILE_RENDERER'), ('', 'generated', '', ''))
        self.assertEqual(self.banks('CFLAGS=-DSC64_TILE_RENDERER=1'), ('', 'generated', '', ''))
        self.assertEqual(self.banks('SHOWCASE_UNIT_WORD_FLAGS=')[-1], '')
        self.assertEqual(self.banks('TERRAN_UNIT_WORD_FLAGS=')[0], '')
        self.assertEqual(self.banks('SHOWCASE_UNIT_WORD_FLAGS=-DSC64_UNIT_WORD_SPRITES=1'), DEFAULT)
