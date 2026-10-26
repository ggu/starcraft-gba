"""Asset-free guards for the source DAT directional-image contract."""
import unittest

from tools.sc64_sprites import IMAGE_COUNT
from tools.sc64_unit_facing import directional_image_flags, keep_directional_bank


class DirectionalMetadataTests(unittest.TestCase):
    def test_images_dat_direction_table_controls_selection(self):
        images_dat = bytearray(IMAGE_COUNT * 5)
        table = IMAGE_COUNT * 4
        # These are the authenticated USA images.dat image IDs. The actual
        # source manifest has 24 Egg frames and 27 Archon frames, despite both
        # flags being clear; Science Vessel is also explicitly non-directional.
        for image_id in (260, 21, 134):
            images_dat[table + image_id] = 0
        images_dat[table + 251] = 1
        flags = directional_image_flags(bytes(images_dat))
        self.assertEqual(flags[260], 0)
        self.assertEqual(flags[21], 0)
        self.assertEqual(flags[134], 0)
        self.assertEqual(flags[251], 1)
        self.assertFalse(keep_directional_bank('science_vessel', bool(flags[260]), 1))
        self.assertFalse(keep_directional_bank('egg', bool(flags[21]), 24))
        self.assertFalse(keep_directional_bank('archon', bool(flags[134]), 27))
        self.assertTrue(keep_directional_bank('marine', bool(flags[251]), 17))

    def test_directional_marker_requires_a_complete_source_bank(self):
        with self.assertRaisesRegex(ValueError, 'only 16 frames'):
            keep_directional_bank('fixture', True, 16)
        self.assertTrue(keep_directional_bank('siege_tank_mode', False, 6))


if __name__ == '__main__':
    unittest.main()
