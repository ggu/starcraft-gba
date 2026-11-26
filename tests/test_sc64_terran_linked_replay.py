# SPDX-License-Identifier: GPL-3.0-or-later
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import sc64_terran_linked_replay as linked


class CheckpointTests(unittest.TestCase):
    def test_t6_preserves_all_inherited_bits_and_requires_new_bit(self):
        previous = (1 << 4) | (1 << 8) | (1 << 27)
        row = {'race_completed': 1 << 9}
        linked.compare(6, row, {'race_completed': previous | (1 << 9)}, previous, -1)
        for got in (1 << 9, previous, previous | (1 << 9) | (1 << 11)):
            with self.assertRaises(RuntimeError):
                linked.compare(6, row, {'race_completed': got}, previous, -1)

    def test_t7_checks_all_fields_and_tracked_carrier(self):
        row = dict.fromkeys(linked.T7_FIELDS, 0)
        row.update(race_completed=1 << 10, tracked_carrier=12)
        previous = sum(1 << i for i in range(4, 10)) | (1 << 29)
        got = {k: row[k] for k in linked.T7_FIELDS}
        got['race_completed'] |= previous
        linked.compare(7, row, got, previous, 12)
        for key in linked.T7_FIELDS:
            changed = dict(got)
            changed[key] += 1
            with self.assertRaises(ValueError):
                linked.compare(7, row, changed, previous, 12)
        with self.assertRaises(ValueError):
            linked.compare(7, row, got, previous, 13)
        with self.assertRaises(ValueError):
            linked.compare(7, {k: v for k, v in row.items() if k != 'race_completed'}, got, previous, 12)


    def test_t8_checks_expected_fields_and_retains_prior_bits(self):
        row = dict.fromkeys(linked.T8_FIELDS, 0)
        previous = sum(1 << i for i in range(4, 11)) | (1 << 28)
        row['race_completed'] = 1 << 11
        got = dict(row, race_completed=previous | (1 << 11))
        linked.compare(8, row, got, previous, -1)
        got['hostile_buildings'] += 1
        with self.assertRaises(ValueError):
            linked.compare(8, row, got, previous, -1)

    def test_t9_checks_every_field_with_inherited_bits(self):
        row = dict.fromkeys(linked.T9_FIELDS, 0)
        previous = sum(1 << i for i in range(4, 12)) | (1 << 28)
        row['race_completed'] = 1 << 12
        got = dict(row, race_completed=previous | (1 << 12))
        linked.compare(9, row, got, previous, -1)
        for field in linked.T9_FIELDS:
            changed = dict(got)
            changed[field] += 1
            with self.assertRaises(ValueError):
                linked.compare(9, row, changed, previous, -1)

    def test_t10_checks_every_field_with_inherited_bits(self):
        row = dict.fromkeys(linked.T10_FIELDS, 0)
        previous = sum(1 << i for i in range(4, 13)) | (1 << 28)
        row['race_completed'] = 1 << 13
        got = dict(row, race_completed=previous | (1 << 13))
        linked.compare(10, row, got, previous, -1)
        for field in linked.T10_FIELDS:
            changed = dict(got)
            changed[field] += 1
            with self.assertRaises(ValueError):
                linked.compare(10, row, changed, previous, -1)


if __name__ == '__main__':
    unittest.main()
