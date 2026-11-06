#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Reject incomplete facts before emitting accessors with fixed raw-ID bounds."""
import copy
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location('stock_gen', ROOT / 'tools/sc64_stock_profiles_generate.py')
GEN = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(GEN)


class UpgradeFamilyGeneration(unittest.TestCase):
    def setUp(self):
        self.facts = {
            key: [{'id': i, 'name': f'ROW_{i}', field: {'id': 61}}
                  for i in range(count)]
            for key, field, count in (('units', 'armor_upgrade', 234),
                                      ('weapons', 'upgrade_type', 132))}

    def emit(self, facts):
        with tempfile.TemporaryDirectory() as directory:
            source, output = Path(directory) / 'facts.json', Path(directory) / 'out.h'
            source.write_text(json.dumps(facts))
            try:
                GEN.emit_upgrade_families(source, output)
            except ValueError:
                self.assertFalse(output.exists())
                raise
            self.assertTrue(output.is_file())

    def test_complete_input(self):
        self.emit(self.facts)

    def test_incomplete_arrays(self):
        for key in ('units', 'weapons'):
            for count in (0, 1, len(self.facts[key]) - 1):
                with self.subTest(key=key, count=count):
                    facts = copy.deepcopy(self.facts)
                    facts[key] = facts[key][:count]
                    with self.assertRaises(ValueError):
                        self.emit(facts)

    def test_bad_ids(self):
        for key, field in (('units', 'armor_upgrade'), ('weapons', 'upgrade_type')):
            for value in (-1, 63, True):
                with self.subTest(key=key, value=value):
                    facts = copy.deepcopy(self.facts)
                    facts[key][0][field]['id'] = value
                    with self.assertRaises(ValueError):
                        self.emit(facts)
            facts = copy.deepcopy(self.facts)
            facts[key][1]['id'] = 0
            with self.assertRaises(ValueError):
                self.emit(facts)


if __name__ == '__main__':
    unittest.main()
