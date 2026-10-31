# SPDX-License-Identifier: GPL-3.0-or-later
"""Asset-free Scarab actor graph and directional export contract."""
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch
from tools import sc64_sprites as base
from tools import sc64_race_sprites as races
from tools import sc64_unit_facing as facing

class ScarabSourceArtTest(unittest.TestCase):
    def test_dat_chain_is_actor_identity_not_weapon_graphic(self):
        units = bytearray(base.UNIT_COUNT)
        flingy = bytearray(base.FLINGY_COUNT * 2)
        sprites = bytearray(base.SPRITE_COUNT * 2)
        images = bytearray(base.IMAGE_COUNT * 4)
        units[85] = 48
        flingy[48 * 2:48 * 2 + 2] = (200).to_bytes(2, 'little')
        sprites[200 * 2:200 * 2 + 2] = (147).to_bytes(2, 'little')
        images[147 * 4:147 * 4 + 4] = (153).to_bytes(4, 'little')
        tables = {'00C/001': bytes(units), '00C/002': bytes(flingy),
                  '00C/003': bytes(sprites), '00C/000': bytes(images)}
        with patch.object(base, '_entry_payload', side_effect=lambda archive, path: tables[path]):
            self.assertEqual(base.resolve_grp_indices(object(), {'scarab': 85})['scarab'],
                {'unit_id': 85, 'flingy_id': 48, 'sprite_id': 200, 'image_id': 147,
                 'grp_filename_index': 153, 'bolt_path': '00D/098'})
            broken = bytearray(flingy); broken[96:98] = (base.SPRITE_COUNT).to_bytes(2, 'little')
            tables['00C/002'] = bytes(broken)
            with self.assertRaisesRegex(ValueError, 'invalid sprite'):
                base.resolve_grp_indices(object(), {'scarab': 85})

    def test_shared_and_directional_exports_include_raw_85(self):
        self.assertEqual(races.RACE_UNIT_IDS['scarab'], 85)
        self.assertEqual(facing.unit_ids()['scarab'], 85)
        self.assertTrue(facing.keep_directional_bank('scarab', True, 17))
        with self.assertRaisesRegex(ValueError, 'only 16 frames'):
            facing.keep_directional_bank('scarab', True, 16)
        frame = {'pixels': bytes((9,)), 'alpha': bytes((1,)), 'width': 1,
                 'height': 1, 'x': 0, 'y': 0, 'frame_width': 1, 'frame_height': 1}
        with tempfile.TemporaryDirectory() as d:
            output = Path(d) / 'sprites.h'
            races.write_header(output, [{'name': name, 'frame': frame}
                                        for name in races.RACE_UNIT_IDS], bytes(range(256)))
            text = output.read_text()
            self.assertIn('case 85: return &sc64_race_sprite_scarab;', text)
            self.assertIn('sc64_race_sprite_scarab_pixels', text)

if __name__ == '__main__': unittest.main()
