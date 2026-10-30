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

    def test_negative_lo_dimensions_fail_closed(self):
        f = fixture()
        f[5][0].update(frame_count=-1, offset_count=-1)
        self.assertEqual(_profile_cull_bounds(*f), [INVALID])

if __name__ == '__main__':
    unittest.main()
