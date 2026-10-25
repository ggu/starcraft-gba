import importlib.util
from pathlib import Path
import struct
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    'sc64_z5_doodad_pack', ROOT / 'tools/sc64_z5_doodad_pack.py')
PACK = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(PACK)


def row(raw_type, x, y, owner, unused, flags):
    return struct.pack('<HHHBBH', raw_type, x, y, owner, unused, flags)


class Z5DoodadPackTests(unittest.TestCase):
    def test_preserves_all_rows_and_classifies_only_sprite_flag(self):
        payload = (row(203, 100, 200, 7, 0, 0x0000) +
                   row(205, 300, 400, 0, 2, 0x8000) +
                   row(127, 500, 600, 8, 0, 0x1000) +
                   row(213, 700, 800, 9, 1, 0x2000))
        decoded = PACK.decode_thg2(payload, 32, 32)
        self.assertEqual(len(decoded), 4)
        self.assertEqual([r['index'] for r in decoded], list(range(4)))
        self.assertEqual([r['type'] for r in decoded], [203, 205, 127, 213])
        self.assertEqual([r['unit_backed'] for r in decoded], [1, 1, 0, 1])
        self.assertEqual([r['initial_enabled'] for r in decoded], [1, 0, 0, 1])
        self.assertEqual((decoded[0]['owner'], decoded[0]['unused']), (7, 0))
        self.assertEqual(decoded[3]['flags'], 0x2000)
        header = PACK.emit_header({'global_catalog_id': 16, 'width': 128,
            'height': 128, 'unit_backed_count': 3, 'sprite_count': 1,
            'doodads': decoded})
        self.assertIn('SC64_Z5_DOODAD_GLOBAL_CATALOG_ID 16u', header)
        self.assertIn('SC64_Z5_DOODAD_COUNT 4u', header)
        self.assertIn('{213u,700u,800u,9u,1u,8192u,1u,1u}', header)
        self.assertNotIn('string', header.lower())

    def test_rejects_partial_records_and_invalid_coordinates(self):
        with self.assertRaisesRegex(ValueError, 'partial source record'):
            PACK.decode_thg2(b'bad', 128, 128)
        with self.assertRaisesRegex(ValueError, 'outside map bounds'):
            PACK.decode_thg2(row(203, 128 * 32, 10, 0, 0, 0), 128, 128)
        with self.assertRaisesRegex(ValueError, 'outside imported-map bounds'):
            PACK.decode_thg2(b'', 129, 128)

    def test_all_rows_are_kept_when_capacity_exceeds_actor_subset(self):
        payload = b''.join(row(127 if i < 30 else 203, i, i, 0, 0,
                               0x1000 if i < 30 else 0) for i in range(71))
        decoded = PACK.decode_thg2(payload, 128, 128)
        self.assertEqual(len(decoded), 71)
        self.assertEqual(sum(r['unit_backed'] for r in decoded), 41)
        self.assertEqual(sum(not r['unit_backed'] for r in decoded), 30)
        self.assertEqual(decoded[-1]['index'], 70)


if __name__ == '__main__':
    unittest.main()
