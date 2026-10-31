"""Synthetic format checks; no ROM or extracted graphics are required."""
import importlib.util
from pathlib import Path
import struct
import unittest

spec = importlib.util.spec_from_file_location('sc64_assets', Path(__file__).resolve().parents[1] / 'tools/sc64_assets.py')
assets = importlib.util.module_from_spec(spec)
spec.loader.exec_module(assets)


class TerrainTests(unittest.TestCase):
    def test_cover_reads_cv5_group_not_variant_or_vf4(self):
        cv5 = bytearray(52 * 3)
        struct.pack_into('<H', cv5, 52 + 2, 0x10)
        self.assertEqual([assets.cv5_cover(16 + variant, cv5) for variant in range(16)], [1] * 16)
        self.assertEqual(assets.cv5_cover(32, cv5), 0)
        self.assertEqual(assets.cv5_cover(0x8000 | 16, cv5), 1)
        with self.assertRaisesRegex(ValueError, 'CV5 flags outside'):
            assets.cv5_cover(48, cv5)

    def test_minitile_order_flip_and_walk_flags(self):
        cv5 = bytearray(52)
        vx4 = struct.pack('<16H', *[i & 1 for i in range(16)])
        vf4 = struct.pack('<16H', *[1 if i in (0, 5, 15) else 0 for i in range(16)])
        pixels, mask = assets.decode_tile(0, cv5, vx4, vf4, bytes(range(16)))
        self.assertEqual(pixels[:8], bytes([0, 1, 2, 3, 3, 2, 1, 0]))
        self.assertEqual(pixels[16:24], bytes([4, 5, 6, 7, 7, 6, 5, 4]))
        self.assertEqual(mask, (1 << 0) | (1 << 5) | (1 << 15))
        with self.assertRaises(ValueError):
            assets.decode_tile(16, cv5, vx4, vf4, bytes(range(16)))
        with self.assertRaises(ValueError):
            assets.decode_tile(0, cv5, struct.pack('<16H', *[2] * 16), vf4, bytes(range(16)))

    def test_palette_preserves_ui_reservation_and_channels(self):
        source = struct.pack('>256H', *([0xF801, 0x07C1, 0x003F, 0xFFFF] * 64))
        colors = assets.palette_rgb555(source)
        self.assertEqual(colors[:4], [31, 31 << 5, 31 << 10, 32767])
        converted, remap = assets.compact_palette(colors, assets.Counter(range(256)))
        self.assertEqual(len(converted), 256)
        self.assertTrue(all(v >= 32 for v in remap))
        self.assertEqual([converted[remap[i]] for i in range(4)], colors[:4])

    def test_section_bounds(self):
        data = struct.pack('<4sI', b'DIM ', 4) + struct.pack('<HH', 64, 64)
        self.assertEqual(assets.sections(data)[b'DIM '], struct.pack('<HH', 64, 64))
        with self.assertRaises(ValueError):
            assets.sections(data[:-1])


if __name__ == '__main__':
    unittest.main()
