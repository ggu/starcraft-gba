"""Authenticated upgrade table decoding and fail-closed boundary."""
import hashlib
import struct
import unittest
from unittest.mock import patch
from tools import sc64_protoss_research_facts as facts

class ProtossResearchFactsTest(unittest.TestCase):
    def table(self):
        data = bytearray(facts.COUNT * 21)
        for ident in facts.ROWS:
            for col in range(6):
                struct.pack_into('<H', data, 2 * (facts.COUNT * col + ident),
                                 1000 * col + ident + 1)
            data[facts.COUNT * 19 + ident] = 1 if ident == 33 else 3
        return bytes(data)

    def test_column_major_rows_and_levels(self):
        data = self.table()
        with patch.object(facts, 'TABLE_SHA256', hashlib.sha256(data).hexdigest()):
            rows = facts.decode(data)
        self.assertEqual(rows['ground_weapons'], {
            'source_id': 13, 'minerals_base': 14, 'minerals_factor': 1014,
            'gas_base': 2014, 'gas_factor': 3014, 'time_base': 4014,
            'time_factor': 5014, 'max_level': 3})
        self.assertEqual(rows['singularity_charge']['max_level'], 1)
        self.assertEqual(len(rows), 6)

    def test_bad_digest_or_length_refused(self):
        with self.assertRaisesRegex(ValueError, 'authentication'):
            facts.decode(self.table())
        with self.assertRaisesRegex(ValueError, 'authentication'):
            facts.decode(b'')

    def test_associations_use_weapon_and_armor_source_columns(self):
        units, weapons = bytearray(19876), bytearray(5460)
        for ident in facts.ASSOCIATION_UNITS:
            units[5892 + ident] = 130
            units[6348 + ident] = 130
        units[8400 + 66] = 5
        units[5892 + 66] = units[6348 + 66] = 66
        weapons[2210 + 66] = 13
        units[8400 + 70] = 6
        units[5892 + 70], units[6348 + 70] = 73, 74
        weapons[2210 + 73] = weapons[2210 + 74] = 14
        with patch.object(facts, 'UNIT_SHA256', hashlib.sha256(units).hexdigest()), \
             patch.object(facts, 'WEAPON_SHA256', hashlib.sha256(weapons).hexdigest()):
            rows = facts.decode_associations(bytes(units), bytes(weapons))
        self.assertEqual(rows['66']['air_upgrade'], 13)
        self.assertEqual(rows['70']['ground_upgrade'], 14)
        self.assertEqual(rows['70']['armor_upgrade'], 6)
        self.assertIsNone(rows['64']['air_upgrade'])
        with self.assertRaisesRegex(ValueError, 'authentication'):
            facts.decode_associations(bytes(units), bytes(weapons))

    def test_authenticated_invalid_level_refused(self):
        data = bytearray(self.table())
        data[facts.COUNT * 19 + 5] = 0
        with patch.object(facts, 'TABLE_SHA256', hashlib.sha256(data).hexdigest()):
            with self.assertRaisesRegex(ValueError, 'invalid upgrade row'):
                facts.decode(data)

if __name__ == '__main__':
    unittest.main()
