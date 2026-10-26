# SPDX-License-Identifier: GPL-3.0-or-later
import struct
import unittest
from unittest.mock import patch

from tools import sc64_effect_palette as mod


class EffectPaletteTests(unittest.TestCase):
    def test_rows_preserve_source_lookup_and_installed_palette(self):
        colors = [(i & 31, (i >> 3) & 31, (i * 7) & 31) for i in range(256)]
        source = b"".join(struct.pack(">H", r << 11 | g << 6 | b << 1)
                          for r, g, b in colors)
        # Deliberately reorder actual colors, which invalidates an identity
        # source-index inverse even when every color is exactly representable.
        actual = list(reversed(colors))
        light = bytes((col + row * 11) & 255
                      for row in range(63) for col in range(256))
        with patch.object(mod, "_actual_gba_palette", return_value=(actual, {})):
            got = mod.project_light_table(source, light, [0] * 256)
        wanted = bytes(255 - ((255 - col + row * 11) & 255)
                       for row in range(63) for col in range(256))
        self.assertEqual(got, wanted)

    def test_bad_dimensions_and_values_fail_closed(self):
        for source, table, palette in ((bytes(511), bytes(256), [0] * 256),
                                       (bytes(512), b"", [0] * 256),
                                       (bytes(512), bytes(257), [0] * 256),
                                       (bytes(512), bytes(65536), [0] * 256),
                                       (bytes(512), bytes(256), [0] * 255),
                                       (bytes(512), bytes(256), [-1] * 256)):
            with self.subTest(length=len(table)), self.assertRaises(ValueError):
                mod.project_light_table(source, table, palette)


if __name__ == "__main__":
    unittest.main()
