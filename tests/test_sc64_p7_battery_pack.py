import hashlib
from pathlib import Path
import struct
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

from tools import sc64_p7_battery_pack as pack


def unit(raw=172, owner=2, x=96, y=160, valid=15, hp=75, shields=60, energy=50):
    row = bytearray(36)
    struct.pack_into('<HHH', row, 4, x, y, raw)
    struct.pack_into('<H', row, 14, valid)
    row[16:20] = bytes((owner, hp, shields, energy))
    return bytes(row)


class BatteryPropertyTests(unittest.TestCase):
    def test_preserves_source_row_coordinates_and_percentages(self):
        rows = pack.decode_batteries(unit(raw=65) + unit(), 32, 32)
        self.assertEqual(rows, [{'row': 1, 'x_source': 96, 'y_source': 160,
                                'valid': 15, 'owner': 2, 'hp': 75,
                                'shields': 60, 'energy': 50}])

    def test_rejects_empty_and_truncated_unit_sections(self):
        for payload in (b'', unit()[:-1], unit() + b'x'):
            with self.subTest(size=len(payload)), self.assertRaises(ValueError):
                pack.decode_batteries(payload, 32, 32)

    def test_rejects_outside_owner_and_half_open_map_bounds(self):
        for change in ({'owner': 8}, {'x': 1024}, {'y': 1024}):
            with self.subTest(change=change), self.assertRaises(ValueError):
                pack.decode_batteries(unit(**change), 32, 32)
        self.assertEqual(len(pack.decode_batteries(unit(x=1023, y=1023), 32, 32)), 1)

    def test_rejects_missing_property_bits_and_invalid_percentages(self):
        for change in ({'valid': 7}, {'valid': 31}, {'hp': 101},
                       {'shields': 101}, {'energy': 101}):
            with self.subTest(change=change), self.assertRaises(ValueError):
                pack.decode_batteries(unit(**change), 32, 32)

    def test_rejects_invalid_map_dimensions(self):
        for width, height in ((0, 32), (32, -1), (129, 32)):
            with self.subTest(shape=(width, height)), self.assertRaises(ValueError):
                pack.decode_batteries(unit(), width, height)

    def test_authentication_failure_does_not_create_output(self):
        with tempfile.TemporaryDirectory() as temp:
            out = Path(temp) / 'generated.h'
            decoder = SimpleNamespace(parse_source=lambda *_: (_ for _ in ()).throw(
                ValueError('source authentication failed')))
            with patch.object(pack, '_scenario_packer', return_value=decoder):
                with self.assertRaisesRegex(ValueError, 'authentication'):
                    pack.generate(Path(temp), out)
            self.assertFalse(out.exists())

    def test_rejects_changed_chk_after_authentication(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            (root / 'p7').mkdir()
            (root / 'p7/source.chk').write_bytes(b'changed')
            scenario = {'source_chk_sha256': hashlib.sha256(b'original').hexdigest()}
            decoder = SimpleNamespace(parse_source=lambda *_: scenario)
            with patch.object(pack, '_scenario_packer', return_value=decoder):
                with self.assertRaisesRegex(ValueError, 'changed'):
                    pack.parse_source(root)


if __name__ == '__main__':
    unittest.main()
