# SPDX-License-Identifier: GPL-3.0-or-later
"""Check lookup dependencies and mode changes against the production Makefile."""
from pathlib import Path
import re
import subprocess
import tempfile
import unittest

from tests.test_sc64_weapon_lookup_index import BANK

ROOT = Path(__file__).resolve().parents[1]
INDEX = 'build/generated/weapon-lookup/sc64_weapon_source_indexed.h'
CONFIG = 'build/canonical-lookup-config'


def database():
    result = subprocess.run(['make', '-pRrq', '-f', 'Makefile'], cwd=ROOT,
                            text=True, capture_output=True)
    if result.returncode not in (0, 1):
        raise AssertionError(result.stderr)
    rules, current = {}, None
    for line in result.stdout.split('# Files\n', 1)[1].splitlines():
        if line and not line.startswith((' ', '\t', '#')) and ':' in line and ':=' not in line:
            name, dependencies = line.split(':', 1)
            if '=' in name or '%' in name:
                current = None
                continue
            current = name
            rules[current] = {'deps': dependencies.split(), 'recipes': []}
        elif line.startswith('\t') and current:
            rules[current]['recipes'].append(line)
    return rules


def reachable(rules, target, dependency, seen=None):
    seen = set() if seen is None else seen
    if target in seen:
        return False
    seen.add(target)
    return any(child == dependency or reachable(rules, child, dependency, seen)
               for child in rules.get(target, {}).get('deps', []) if child != '|')


class LookupMakeTest(unittest.TestCase):
    def test_canonical_consumers_depend_on_bank_and_mode(self):
        rules = database()
        flags = re.compile(r'TERRAN_ROM_FLAGS|ZERG_ROM_FLAGS|PROTOSS_(?:ROM_FLAGS|P7_CANONICAL_FLAGS)|'
                           r'SHOWCASE_FLAGS|SC64_WEAPON_SOURCE_BANK_HEADER|'
                           r'CONSTRUCTION_ARRIVAL_FLAGS_(?:terran|zerg-z6|protoss-p7)')
        consumers = [target for target, rule in rules.items()
                     if any(flags.search(recipe) for recipe in rule['recipes'])]
        self.assertGreater(len(consumers), 4)
        for target in consumers:
            with self.subTest(target=target):
                self.assertTrue(reachable(rules, target, INDEX), 'selected header can be absent or stale')
                self.assertTrue(reachable(rules, target, CONFIG), 'mode changes must rebuild consumers')
        self.assertFalse(reachable(rules, 'build/starcraft.elf', INDEX))
        self.assertFalse(reachable(rules, 'build/starcraft.elf', CONFIG))

    def test_mode_switch_and_older_alternate_bank(self):
        # Run the real rules with synthetic private inputs, no asset generators.
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'Makefile').write_bytes((ROOT / 'Makefile').read_bytes())
            (root / 'tools').symlink_to(ROOT / 'tools', target_is_directory=True)
            for name, bank in [('first', BANK), ('second', BANK.replace('{741}', '{740}').replace('{741,255}', '{740,255}'))]:
                (root / name).mkdir()
                (root / name / 'sc64_weapon_source_generated.h').write_text(bank)
            (root / 'fixture.mk').write_text('''lookup-fixture: build/lookup-consumer
build/lookup-consumer: $(CANONICAL_LOOKUP_INPUTS)
\t@printf 'rebuilt\\n' >> build/rebuilds
\t@touch $@
''')

            def build(mode, bank='first', route=1):
                subprocess.run(['make', '-f', 'Makefile', '-f', 'fixture.mk',
                                f'CANONICAL_LOOKUP_INDEX={mode}', f'CANONICAL_ROUTE_INDEX={route}', f'WEAPON_SOURCE_DIR={bank}',
                                '-o', f'{bank}/sc64_weapon_source_generated.h', 'lookup-fixture'],
                               cwd=root, check=True, capture_output=True)
                return (root / 'build/rebuilds').read_text().splitlines()

            self.assertEqual(len(build(1)), 1)
            original = (root / INDEX).read_bytes()
            self.assertEqual(len(build(1)), 1, 'unchanged configuration must not rebuild')
            self.assertEqual(len(build(0)), 2)
            self.assertEqual((root / INDEX).read_bytes(), original)
            self.assertEqual(len(build(1)), 3)
            self.assertEqual(len(build(1, 'second')), 4, 'bank path changes must invalidate even older inputs')
            self.assertNotEqual((root / INDEX).read_bytes(), original)
            self.assertEqual(len(build(1, 'second')), 4)
            # A same-path edit within one filesystem clock tick must also
            # invalidate the selected bank on macOS Make 3.81.
            (root / 'second/sc64_weapon_source_generated.h').write_text(BANK)
            self.assertEqual(len(build(1, 'second')), 5)
            self.assertEqual((root / INDEX).read_bytes(), original)
            self.assertEqual(len(build(1, 'second')), 5)
            self.assertEqual(len(build(1, 'second', 0)), 6)
            self.assertEqual(len(build(1, 'second', 1)), 7)
            self.assertEqual(len(build(1, 'second', 1)), 7)


if __name__ == '__main__':
    unittest.main()
