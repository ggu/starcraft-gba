import tempfile
import unittest
from pathlib import Path

from tools.sc64_race_sprites import MISSION_RAW_TYPES, RACE_UNIT_IDS, write_header


class RaceSpritePackTests(unittest.TestCase):
    def test_campaign_raw_types_have_real_asset_mappings(self):
        self.assertEqual(set(MISSION_RAW_TYPES) - set(RACE_UNIT_IDS.values()), set())
        self.assertEqual(RACE_UNIT_IDS['drone'], 41)
        self.assertEqual(RACE_UNIT_IDS['probe'], 64)
        self.assertEqual(RACE_UNIT_IDS['zealot'], 65)
        self.assertEqual(RACE_UNIT_IDS['hydralisk'], 38)
        self.assertEqual(RACE_UNIT_IDS['photon_cannon'], 162)

    def test_header_remaps_opaque_indices_and_keeps_transparency_zero(self):
        frame = {'pixels': bytes((0, 9, 0, 7)), 'alpha': bytes((0, 1, 0, 1)),
                 'width': 2, 'height': 2, 'x': 0, 'y': 0,
                 'frame_width': 2, 'frame_height': 2}
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / 'sprites.h'
            remap = bytes((i + 32) & 255 for i in range(256))
            write_header(path, [{'name': 'test', 'frame': frame}], remap)
            text = path.read_text()
        self.assertIn('sc64_race_sprite_test_pixels[4]', text)
        self.assertIn('0x00,0x29,0x00,0x27', text)
        self.assertIn('{0,0,2,2,2,2,sc64_race_sprite_test_pixels}', text)


if __name__ == '__main__':
    unittest.main()
