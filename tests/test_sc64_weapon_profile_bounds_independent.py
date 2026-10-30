# SPDX-License-Identifier: GPL-3.0-or-later
"""Independent coverage for source actor overlays and malformed LO tables."""
import copy
import pathlib
import sys
import unittest
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from tools.sc64_weapon_source_sprites import _profile_cull_bounds
from test_sc64_weapon_profile_bounds import fixture, INVALID

class IndependentBoundsTest(unittest.TestCase):
    def test_actor_art_is_not_drawn_but_actor_child_is(self):
        f = fixture()
        # The actor frame need not contribute to the envelope. Its overlay does.
        f[2][1].update(x=5000, mirror_x=-5000)
        result = _profile_cull_bounds(*f)[0]
        self.assertNotEqual(result, INVALID)
        self.assertEqual(result & 0x7fff, 23)
        f[2][2].update(x=100)
        result = _profile_cull_bounds(*f)[0]
        self.assertGreaterEqual(result & 0x7fff, 122)

    def test_only_selected_parent_offsets_contribute(self):
        f = fixture()
        for overlay in f[4]: overlay['lo_index'] = 1
        f[5][0].update(image_id=10, lo_kind=1)
        baseline = _profile_cull_bounds(*f)[0]
        f[4].append({'parent_image':30,'image_id':30,'opcode':8,'x':30000,'y':-30000,'lo_index':1})
        f[3].append({'image_id':30,'animation_id':0,'overlay_first':2,'overlay_count':1,'event_first':0,'event_count':0})
        f[6].append({'x':127,'y':-128})
        f[5].append({'image_id':30,'lo_kind':1,'offset_first':1,'frame_count':1,'offset_count':1})
        self.assertEqual(_profile_cull_bounds(*f)[0], baseline)

    def test_all_matching_lo_birth_offsets_are_included(self):
        for opcode in (13, 14, 21):
            f = fixture()
            for overlay in f[4]: overlay['lo_index'] = 1
            f[4][1]['opcode'] = opcode
            f[5][0].update(image_id=10, lo_kind=1, frame_count=2)
            f[6].append({'x':-127,'y':-128})
            result = _profile_cull_bounds(*f)[0]
            self.assertGreaterEqual(result & 0x7fff, 66)
            self.assertGreaterEqual((result >> 15) & 0x7fff, 66)

    def test_negative_lo_dimensions_fail_closed(self):
        f = fixture()
        f[5][0].update(frame_count=-1, offset_count=-1)
        self.assertEqual(_profile_cull_bounds(*f), [INVALID])

if __name__ == '__main__':
    unittest.main()
