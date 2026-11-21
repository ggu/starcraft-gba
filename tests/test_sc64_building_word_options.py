# SPDX-License-Identifier: GPL-3.0-or-later
"""Check the shared building compositor selection without game assets."""

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


class BuildingWordOptionsTest(unittest.TestCase):
    def plan(self, *settings):
        format_args = ' '.join(f"'$({variable})'" for variable in BANKS)
        recipe = f'.PHONY: print-building-banks\nprint-building-banks:\n\t@printf "%s\\n" {format_args}\n'
        return subprocess.run(
            ['make', '-s', '-f', 'Makefile', '-f', '-', *settings, 'print-building-banks'],
            input=recipe, cwd=ROOT, text=True, capture_output=True, check=False,
        )

    def banks(self, *settings):
        plan = self.plan(*settings)
        self.assertEqual(plan.returncode, 0, plan.stderr)
        return tuple(Path(line).parent.name if line else '' for line in plan.stdout.splitlines())

    def test_normal_and_rollback_use_the_expected_banks(self):
        self.assertEqual(self.banks(), PACKED)
        self.assertEqual(self.banks('CANONICAL_TERRAN_BUILDING_WORDS=0'), STOCK)
        self.assertNotEqual(self.plan('CANONICAL_TERRAN_BUILDING_WORDS=2').returncode, 0)

    def test_profile_overrides_preserve_other_bank_selections(self):
        for index, option in enumerate(('TERRAN_BUILDING_WORDS', 'Z10_TERRAN_BUILDING_WORDS',
                                        'P10_TERRAN_BUILDING_WORDS', 'SHOWCASE_TERRAN_BUILDING_WORDS')):
            with self.subTest(option=option):
                expected = PACKED[:index] + (STOCK[index],) + PACKED[index + 1:]
                self.assertEqual(self.banks(f'{option}=0'), expected)

    def test_incompatible_renderers_skip_the_shared_extension(self):
        self.assertEqual(self.banks('CFLAGS=-DSC64_TILE_RENDERER'), ('', 'generated', '', ''))
        self.assertEqual(self.banks('SHOWCASE_UNIT_WORD_FLAGS=')[-1], '')
        self.assertEqual(self.banks('TERRAN_UNIT_WORD_FLAGS=')[0], '')
