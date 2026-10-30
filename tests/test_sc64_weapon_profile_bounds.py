# SPDX-License-Identifier: GPL-3.0-or-later
"""Asset-free profile bounds, including malformed metadata fallback."""
import copy
import pathlib
import sys
import unittest
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]))
from tools.sc64_weapon_source_sprites import _profile_cull_bounds

INVALID=0x40000000

def fixture():
    profiles=[{'projectile_image':10,'attack_image':20,'attackwith_tick':2}]
    images=[{'image_id':10,'frame_first':0,'frame_count':1,'frame_directory_first':0,'source_frame_count':1},
            {'image_id':20,'frame_first':1,'frame_count':1,'frame_directory_first':1,'source_frame_count':1},
            {'image_id':30,'frame_first':2,'frame_count':1,'frame_directory_first':2,'source_frame_count':1}]
    frames=[{'image_id':10,'x':0,'y':0,'mirror_x':0,'w':4,'h':4,'canvas_w':4,'canvas_h':4},
            {'image_id':20,'x':0,'y':0,'mirror_x':0,'w':2,'h':2,'canvas_w':2,'canvas_h':2},
            {'image_id':30,'x':0,'y':0,'mirror_x':0,'w':6,'h':4,'canvas_w':6,'canvas_h':4}]
    animations=[{'image_id':20,'animation_id':1,'overlay_first':0,'overlay_count':1,'event_first':0,'event_count':0},
                {'image_id':10,'animation_id':0,'overlay_first':1,'overlay_count':1,'event_first':0,'event_count':0}]
    overlays=[{'parent_image':20,'image_id':30,'opcode':8,'x':-10,'y':6},
              {'parent_image':10,'image_id':30,'opcode':21,'x':0,'y':0,'lo_index':3}]
    lo_tables=[{'image_id':10,'lo_kind':3,'offset_first':0,'frame_count':1,'offset_count':1}]
    lo_offsets=[{'x':40,'y':-8}]
    return [profiles,images,frames,animations,overlays,lo_tables,lo_offsets,[],[0,1,2]]

class ProfileBoundsTest(unittest.TestCase):
    def test_children_mirrors_and_birth_offsets_included(self):
        values=_profile_cull_bounds(*fixture());self.assertEqual(values,[0x80000000 | (6<<15) | 23])
    def test_unrelated_offsets_do_not_expand_profile(self):
        f=fixture()
        f[4].append({'parent_image':99,'image_id':30,'opcode':8,'x':30000,'y':30000})
        f[5].append({'image_id':99,'lo_kind':3,'offset_first':1,'frame_count':1,'offset_count':1})
        f[6].append({'x':127,'y':127})
        self.assertEqual(_profile_cull_bounds(*f),[0x80000000 | (6<<15) | 23])
        f[5][0]['image_id']=99
        self.assertEqual(_profile_cull_bounds(*f),[0x80000000 | (5<<15) | 8])

    def test_missing_ambiguous_and_unknown_fall_back(self):
        changes=[lambda f:f[1].pop(),lambda f:f[1].append(copy.deepcopy(f[1][0])),
                 lambda f:f[4][0].update(opcode=99),lambda f:f[3][0].update(overlay_count=99),
                 lambda f:f[4][1].update(parent_image=20),lambda f:f[5][0].update(offset_count=2),
                 lambda f:f[2][2].update(image_id=31),lambda f:f[2][2].update(x=32767,w=100)]
        for change in changes:
            f=fixture();change(f)
            with self.subTest(change=change):self.assertEqual(_profile_cull_bounds(*f),[INVALID])
    def test_unknown_profile_falls_back_independently(self):
        f=fixture();f[0].append({'projectile_image':99,'attack_image':20,'attackwith_tick':2})
        self.assertEqual(_profile_cull_bounds(*f),[0x80000000 | (6<<15) | 23,INVALID])
if __name__=='__main__':unittest.main()
