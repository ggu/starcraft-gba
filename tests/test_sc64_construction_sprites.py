#!/usr/bin/env python3
"""Asset-free tests for the construction image header writer."""
from __future__ import annotations
import struct
import subprocess
import tempfile
import unittest
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
from tools import sc64_construction_sprites as mod


EXPECTED_OP_FORMATS = (
    "2", "2", "s1", "s1", "s1s1", "1", "11", "j", "211", "211",
    "2", "2", "", "211", "211", "2s1s1", "211", "211", "2", "211",
    "211", "21", "e", "1", "2", "v", "22", "", "v", "",
    "1b", "1", "1", "", "1", "1", "1", "1", "", "",
    "1", "1", "", "1", "1", "", "", "", "", "1",
    "", "", "1", "b", "", "2", "1", "b", "2b", "22b",
    "22b", "11", "", "b", "2", "1", "211", "", "",
)

class ImportTests(unittest.TestCase):
    def test_flip_audit_visits_conditional_branch_targets(self):
        # op30 has a conditional jump to the setflipstate on the non-fallthrough
        # path; construction code must not silently lose that visual state.
        code = bytes((30, 0, 5, 0, 22, 23, 1, 22))
        source = [{"image_id": 10, "animation": "Init", "start": 0}]
        with self.assertRaisesRegex(ValueError, "setflipstate"):
            mod.audit_construction_flipstate(code, source, {(10, "Init"): 0}, [0])

    def test_truncated_script_records_fail_closed(self):
        for payload, offset in ((b"SCPE", 0), (b"", 1), (b"SCPE"+bytes(4), -1)):
            with self.subTest(payload=payload, offset=offset), self.assertRaises(ValueError):
                mod.image_animation_starts(payload, offset)

    def test_tables_select_construction_and_normal_art_without_facts(self):
        units = bytearray(228 * 14)
        struct.pack_into("<I", units, 228 * 5 + 96 * 2 + 106 * 4, 325)
        units[106] = 1
        flingy = bytearray(209 * 2)
        struct.pack_into("<H", flingy, 2, 1)
        sprites = bytearray(517 * 2)
        struct.pack_into("<H", sprites, 2, 275)
        images = bytearray(999 * 38)
        struct.pack_into("<I", images, 0, 1)
        struct.pack_into("<I", images, 275 * 4, 12)
        struct.pack_into("<I", images, 325 * 4, 13)
        tables = mod.table_images(bytes(units), bytes(flingy), bytes(sprites), bytes(images))
        payload = struct.pack("<IhH", 4, -1, 0)
        found = mod.select_images(lambda path: payload, tables)
        self.assertIn(325, found)
        self.assertEqual(found[325]["grp_path"], "00D/00C")
        self.assertIn("construction_image", found[325]["roles"])
        self.assertIn("normal_structure", found[275]["roles"])

    def test_bounded_iscript_reader_follows_goto_and_stops_on_loop(self):
        program = bytes([0, 1, 0, 5, 2, 7, 0, 0])
        instructions = mod.animation_instructions(program, 0, limit=8)
        self.assertEqual(instructions, [(0, [1]), (5, [2]), (7, [0])])
        self.assertEqual(mod.OP_FORMATS, EXPECTED_OP_FORMATS)
        self.assertEqual(mod.OP_FORMATS[8], "211")

    def test_stage_inputs_use_morph_special_state_one_as_initial(self):
        units = bytearray(228 * 14)
        construction_offset = 228 * 5 + 96 * 2
        for raw, image_id in ((106, 325), (131, 104), (132, 103),
                              (133, 103), (137, 103)):
            struct.pack_into("<I", units, construction_offset + raw * 4, image_id)
        units[106] = 1
        flingy = bytearray(209 * 2)
        struct.pack_into("<H", flingy, 2, 1)
        sprites = bytearray(517 * 2)
        struct.pack_into("<H", sprites, 2, 275)
        images = bytearray(999 * 38)
        for image_id in (0, 103, 104, 275, 325):
            struct.pack_into("<I", images, 999 * 10 + image_id * 4, 1)
        tables = mod.table_images(bytes(units), bytes(flingy), bytes(sprites), bytes(images))

        payload = bytearray(84)
        struct.pack_into("<IhHhH", payload, 0, 4, 1, 16, -1, 0)
        payload[16:20] = b"SCPE"
        struct.pack_into("<I", payload, 20, 15)
        for animation_id, start in ((0, 56), (13, 64), (14, 72), (15, 80)):
            struct.pack_into("<H", payload, 24 + animation_id * 2, start)
            payload[start] = 22

        stages = mod.source_stage_inputs(tables, bytes(payload))
        by_key = {(item["raw_unit_id"], item["stage"]): item["animation"] for item in stages}
        self.assertEqual(by_key[(131, 0)], "Init")
        self.assertEqual(by_key[(132, 0)], "SpecialState1")
        self.assertEqual(by_key[(132, 2)], "SpecialState2")
        self.assertEqual(by_key[(133, 0)], "SpecialState1")
        self.assertEqual(by_key[(137, 0)], "SpecialState1")
        self.assertEqual(by_key[(106, 3)], "AlmostBuilt")

    def test_protoss_stage_uses_each_structure_warp_in_animation(self):
        units = bytearray(228 * 14)
        flingy = bytearray(209 * 2)
        sprites = bytearray(517 * 2)
        images = bytearray(999 * 38)
        # Nexus and Pylon resolve through their own DAT image chain.
        for raw, flingy_id, sprite_id, image_id in ((154, 1, 2, 301),
                                                     (156, 3, 4, 302)):
            units[raw] = flingy_id
            struct.pack_into("<H", flingy, flingy_id * 2, sprite_id)
            struct.pack_into("<H", sprites, sprite_id * 2, image_id)
            struct.pack_into("<I", images, 999 * 10 + image_id * 4, 1)
        tables = mod.table_images(bytes(units), bytes(flingy), bytes(sprites), bytes(images))

        # Script id 1 has a WarpIn entry. It is resolved independently for
        # each normal image, even when the IScript record is shared.
        payload = bytearray(72)
        struct.pack_into("<IhHhH", payload, 0, 4, 1, 16, -1, 0)
        payload[16:20] = b"SCPE"
        struct.pack_into("<I", payload, 20, 21)
        struct.pack_into("<H", payload, 24 + 21 * 2, 64)
        payload[64] = 22
        stages = mod.source_stage_inputs(tables, bytes(payload))
        got = {(item["raw_unit_id"], item["image_id"], item["animation"])
               for item in stages}
        self.assertEqual(got, {(154, 301, "WarpIn"), (156, 302, "WarpIn")})

    def test_header_preserves_frame_geometry_and_bounds_lookup(self):
        frame={"x":2,"y":3,"width":8,"height":9,"frame_width":2,"frame_height":2,
               "pixels":bytes([0]*72),"alpha":bytes([0]*72)}
        frame["pixels"]=bytes([i%256 for i in range(72)])
        frame["alpha"]=bytes([1]*72)
        with tempfile.TemporaryDirectory() as d:
            path=Path(d)/"generated.h"
            program = {"raw_unit_id": 10, "stage": 2, "image_id": 44,
                       "program": {"layers": [{"image_id": 44, "x_offset": 0,
                                   "y_offset": 0, "start_source_updates": 0,
                                   "under": False, "follows_main_graphic": False,
                                   "gas_overlay_requests": [],
                                   "program": {"steps": [{"frame": 1, "duration_source_updates": 3}],
                                               "loop_start": 0, "frame_count": 2}}]}}
            mod.write_header(path,[{"image_id":44,"frames":[frame,frame]}],
                             bytes(range(256)),[program])
            self.assertNotIn('#define SC64_SPRITES_GENERATED_H', path.read_text())
            source='''typedef struct { signed short offset_x, offset_y; unsigned short w, h, canvas_w, canvas_h; const unsigned char *pixels; } Sc64SpriteFrame;\n#define SC64_CONSTRUCTION_SOURCE_ART\n#include "generated.h"\nint main(void) { const Sc64SpriteFrame *f=sc64_construction_image_frame(44,1); return !(f && f->offset_x==2 && f->offset_y==3 && f->w==2 && f->h==2 && f->canvas_w==8 && f->canvas_h==9 && sc64_construction_image_frame_count(44)==2 && sc64_construction_image_frame(44,2)==0 && sc64_construction_image_frame(99,0)==0 && f->pixels[0]==26); }\n'''
            source=source.replace('return !(', 'unsigned sampled=99; const SC64ConstructionSourceStage *stage=sc64_construction_stage(10,2); return !(').replace('f->pixels[0]==26);', 'f->pixels[0]==26 && stage && stage->image_id==44 && sc64_construction_animation_frame(stage->animation,100,&sampled) && sampled==1 && !sc64_construction_stage(10,0) && !sc64_construction_stage(-1,2));')
            c=Path(d)/"check.c"; c.write_text(source)
            result=subprocess.run(["cc","-std=c99","-Wall","-Werror","-I",d,"-I",str(ROOT/"starcraft"),str(c),"-o",str(Path(d)/"check")],capture_output=True,text=True)
            self.assertEqual(result.returncode,0,result.stderr)
            run=subprocess.run([str(Path(d)/"check")],capture_output=True,text=True)
            self.assertEqual(run.returncode,0,run.stderr)

if __name__=="__main__": unittest.main()
