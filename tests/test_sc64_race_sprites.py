import tempfile
import unittest
from pathlib import Path

from tools.sc64_race_sprites import (
    MISSION_RAW_TYPES, RACE_UNIT_IDS, T4_EXPECTED_VISIBLE_RAW_TYPES,
    T4_INVISIBLE_RAW_TYPES, T4_UNIT_IDS, asset_manifest_sha256, write_header,
)


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

    def test_t4_assets_and_invisible_revealer_are_profile_gated(self):
        frame = {'pixels': bytes((0, 9, 0, 7)), 'alpha': bytes((0, 1, 0, 1)),
                 'width': 2, 'height': 2, 'x': 0, 'y': 0,
                 'frame_width': 2, 'frame_height': 2}
        t4 = [{'name': name, 'frame': frame} for name in T4_UNIT_IDS]
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / 'sprites.h'
            write_header(path, [], bytes(range(256)), t4)
            text = path.read_text()
        self.assertIn('#ifdef SC64_T4_ENABLED', text)
        self.assertIn('case 15: return &sc64_race_sprite_civilian;', text)
        self.assertIn('case 20: return &sc64_race_sprite_raynor_raw20;', text)
        self.assertIn('case 89: return &sc64_race_sprite_rhynadon;', text)
        self.assertIn('case 95: return &sc64_race_sprite_ragnasaur;', text)
        self.assertIn('case 195: return &sc64_race_sprite_terran_beacon;', text)
        self.assertIn('case 218: return &sc64_race_sprite_data_disk;', text)
        self.assertNotIn('case 101: return &sc64_race_sprite_', text)
        self.assertIn('case 101: return 2; /* source-invisible map revealer */', text)
        self.assertIn('default: return sc64_race_unit_sprite(raw_type) ? 1 : 0;', text)
        self.assertEqual(T4_INVISIBLE_RAW_TYPES, (101,))
        resolved = set(RACE_UNIT_IDS.values()) | set(T4_UNIT_IDS.values())
        self.assertEqual(set(T4_EXPECTED_VISIBLE_RAW_TYPES) - resolved, set())

    def test_asset_manifest_hash_ignores_nonidentity_fields(self):
        asset = {'name': 'x', 'unit_id': 15, 'flingy_id': 1, 'sprite_id': 2,
                 'image_id': 3, 'grp_filename_index': 4, 'bolt_path': '00D/001',
                 'sha1': 'a' * 40, 'canvas': [1, 2], 'selected_frame': 0,
                 'frame_rect': [0, 0, 1, 2], 'opaque_pixels': 2,
                 'preview': 'one.png'}
        before = asset_manifest_sha256([asset])
        asset['preview'] = 'renamed.png'
        self.assertEqual(before, asset_manifest_sha256([asset]))

    def test_base_header_has_no_t4_cases_or_visibility_api(self):
        frame = {'pixels': bytes((0,)), 'alpha': bytes((1,)), 'width': 1,
                 'height': 1, 'x': 0, 'y': 0, 'frame_width': 1,
                 'frame_height': 1}
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / 'sprites.h'
            write_header(path, [{'name': 'marine', 'frame': frame}], bytes(range(256)))
            text = path.read_text()
        self.assertNotIn('SC64_T4_ENABLED', text)
        self.assertNotIn('sc64_race_unit_sprite_mode', text)
        self.assertNotIn('case 218:', text)


if __name__ == '__main__':
    unittest.main()
