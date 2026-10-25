import tempfile
import unittest
from pathlib import Path

from tools.sc64_race_sprites import (
    RACE_UNIT_IDS, ZP_BUILDING_IDS, ZP_INVISIBLE_RAW_TYPES, ZP_UNIT_IDS,
    write_header,
)


def one_frame(pixel=7):
    return {'pixels': bytes((pixel,)), 'alpha': bytes((1,)), 'width': 1,
            'height': 1, 'x': 0, 'y': 0, 'frame_width': 1,
            'frame_height': 1}


class ZpActorSpriteTests(unittest.TestCase):
    def test_campaign_art_and_invisible_actor_use_independent_gate(self):
        units = [{'name': name, 'frame': one_frame()} for name in ZP_UNIT_IDS]
        base_units = [{'name': name, 'frame': one_frame()} for name in RACE_UNIT_IDS]
        buildings = [{'name': name, 'frames': [one_frame(8), one_frame(9)]}
                     for name in ZP_BUILDING_IDS]
        with tempfile.TemporaryDirectory() as temp:
            out = Path(temp) / 'zp.h'
            write_header(out, base_units, bytes(range(256)), zp_selected=units,
                         zp_building_selected=buildings)
            text = out.read_text()
        self.assertIn('#ifdef SC64_ZP_CAMPAIGN_ACTORS', text)
        self.assertIn('#define SC64_ZP_CAMPAIGN_SPRITE_SET_COMPLETE 1', text)
        self.assertIn('case 51: return &sc64_race_sprite_infested_kerrigan;', text)
        self.assertIn('case 19: return &sc64_race_sprite_raynor_vulture_t6;', text)
        self.assertIn('case 79: return &sc64_race_sprite_tassadar_hero_zp;', text)
        self.assertEqual(text.count('case 19: return &sc64_race_sprite_raynor_vulture_t6;'), 1)
        self.assertEqual(text.count('case 79: return &sc64_race_sprite_tassadar_hero_zp;'), 1)
        self.assertIn('case 101: return 2; /* map revealer has no source-visible sprite */', text)
        self.assertIn('case 196:', text)
        self.assertIn('case 1: return &sc64_race_sprite_queens_nest_zp_frame_1;', text)
        self.assertEqual(ZP_INVISIBLE_RAW_TYPES, (101,))
        self.assertNotIn('#ifdef SC64_T4_ENABLED', text)
        self.assertNotIn('#ifdef SC64_TERRAN_LATE_ENABLED', text)


if __name__ == '__main__':
    unittest.main()
