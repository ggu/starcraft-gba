# SPDX-License-Identifier: GPL-3.0-or-later
from __future__ import annotations

import struct
import unittest
from pathlib import Path

from tools import sc64_font_assets as fonts

ROOT = Path(__file__).resolve().parents[1]


def make_font(records: dict[int, bytes], *, low: int = 32, high: int = 36,
              max_width: int = 8, max_height: int = 9) -> bytes:
    count = high - low
    table_end = fonts.FONT_HEADER_BYTES + count * 4
    offsets = [0] * count
    payload = bytearray()
    for code in sorted(records):
        # The low header byte names the unrendered space slot; table entry
        # zero is the first ink glyph at low + 1.
        offsets[code - low - 1] = table_end + len(payload)
        payload.extend(records[code])
    return (b'FONT' + bytes((low, high, max_width, max_height)) + bytes(4) +
            struct.pack(f'<{count}I', *offsets) + payload)


class FontAssetTests(unittest.TestCase):
    def test_decodes_offsets_signed_bearings_and_sparse_pixels(self):
        # Glyph 33: 3x2; 0x01 draws color 1 at pixel 0, 0x17 skips 2
        # then draws color 7 at pixel 3, 0x09 skips one then draws color 1.
        data = make_font({33: bytes((3, 2, 0xFF, 2, 0x01, 0x17, 0x09)),
                          35: bytes((2, 1, 1, 0, 0x01, 0x02))})
        font = fonts.decode_font(data)
        glyph = font.glyphs[33 - font.first_char]
        self.assertEqual((font.first_char, font.end_char), (33, 37))
        self.assertIsNone(font.glyphs[1])
        self.assertEqual((glyph.width, glyph.height, glyph.x_offset, glyph.y_offset),
                         (3, 2, -1, 2))
        self.assertEqual(glyph.pixels, bytes((1, 0, 0, 7, 0, 1)))
        self.assertEqual(glyph.advance, 3)
        self.assertEqual(fonts.font_advance(font, 32), 4)
        self.assertEqual(fonts.font_advance(font, 35), 4)
        self.assertEqual(font.line_height, 10)

    def test_generated_header_preserves_banks_and_alignment(self):
        small = fonts.decode_font(make_font({33: bytes((1, 1, 0, 0, 0x01))}))
        medium = fonts.decode_font(make_font({34: bytes((2, 1, 0, 0, 0x01, 0x01))},
                                             max_width=9, max_height=11))
        header = fonts.build_header(small, medium)
        self.assertIn('sc64_font_small_glyphs', header)
        self.assertIn('sc64_font_medium_glyphs', header)
        self.assertIn('__attribute__((aligned(4)))', header)
        self.assertIn('1,1,0,0,sc64_font_small_33_pixels,2', header)
        self.assertIn('= {33,4,9,11,4,12,sc64_font_medium_glyphs}', header)

    def test_rejects_bad_header_ranges_offsets_and_pixel_runs(self):
        with self.assertRaisesRegex(ValueError, 'invalid header'):
            fonts.decode_font(b'NOPE' + bytes(32))
        with self.assertRaisesRegex(ValueError, 'offset table'):
            fonts.decode_font(b'FONT' + bytes((32, 40, 8, 9)) + bytes(4))

        bad_offset = bytearray(make_font({33: bytes((1, 1, 0, 0, 1))}))
        struct.pack_into('<I', bad_offset, fonts.FONT_HEADER_BYTES + 4, len(bad_offset) + 8)
        with self.assertRaisesRegex(ValueError, 'offset points'):
            fonts.decode_font(bytes(bad_offset))

        # A width-1 glyph cannot skip past its final pixel.
        bad_run = make_font({33: bytes((1, 1, 0, 0, 0x11))})
        with self.assertRaisesRegex(ValueError, 'pixel run exceeds'):
            fonts.decode_font(bad_run)

    @unittest.skipUnless((ROOT / '.local/sc64/rom/StarCraft64-USA.z64').is_file()
                         and (ROOT / 'references/sc64-maps/.git').exists(),
                         'private verified ROM or pinned decoder is unavailable')
    def test_verified_rom_ascii_table_starts_at_first_ink_glyph(self):
        small, _ = fonts.load_source_fonts(
            ROOT / '.local/sc64/rom/StarCraft64-USA.z64',
            ROOT / 'references/sc64-maps')
        self.assertEqual((small.first_char, small.end_char), (33, 240))
        glyph_a = small.glyphs[ord('A') - small.first_char]
        glyph_b = small.glyphs[ord('B') - small.first_char]
        self.assertIsNotNone(glyph_a)
        self.assertIsNotNone(glyph_b)
        self.assertEqual((glyph_a.width, glyph_a.height), (6, 8))
        self.assertNotEqual(glyph_a.pixels, glyph_b.pixels)
        self.assertEqual(fonts.font_advance(small, ord(' ')),
                         max(2, small.max_width // 2))


if __name__ == '__main__':
    unittest.main()
