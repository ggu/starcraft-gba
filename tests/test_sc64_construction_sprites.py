#!/usr/bin/env python3
"""Asset-free tests for the construction image header writer."""
from __future__ import annotations
import struct
import subprocess
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch
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


def synthetic_gba_palette():
    """Return 256 deliberately synthetic RGB555 entries for parser tests."""
    result = [0x7FFF - i for i in range(32)]
    for i in range(224):
        r = (i * 7 + 3) & 31
        g = (i * 11 + 5) & 31
        b = (i * 13 + 9) & 31
        result.append(r | (g << 5) | (b << 10))
    return result


def write_synthetic_palette_sources(root, *, malformed=False):
    common = root / "common"
    starcraft = root / "starcraft"
    common.mkdir(parents=True, exist_ok=True)
    starcraft.mkdir(parents=True, exist_ok=True)
    base_colors = ",".join(f"RGB({i & 31},{(i * 3) & 31},{(i * 5) & 31})"
                            for i in range(32))
    if malformed:
        base_colors += ",NOT_A_COLOR(1,2,3)"
    (common / "gba.c").write_text(
        f"static const u16 colors[32] = {{{base_colors}}};\n", encoding="ascii")
    (common / "gba.h").write_text(
        "enum { C_BLACK=0,C_WHITE,C_DIM,C_PANEL,C_EDGE,C_RED,C_RED_DARK,"
        "C_GREEN,C_GREEN_DARK,C_BLUE,C_BLUE_DARK,C_GOLD,C_GOLD_DARK,C_CYAN,"
        "C_PURPLE,C_ORANGE,C_BROWN,C_DIRT,C_STONE,C_GRASS,C_SAND,C_SILVER,"
        "C_SKIN,C_SHADOW,C_TERRAN,C_ZERG,C_PROTOSS,C_LAVA,C_BLOOD,C_FOG,"
        "C_UI,C_HIGHLIGHT };\n", encoding="ascii")
    lcd = ",".join(str((i * 3 + 1) & 31) for i in range(32))
    overrides = "{C_DIM,21,23,25},{C_GRASS,8,15,9},{C_ZERG,25,11,22}"
    (starcraft / "sc64_display.h").write_text(
        "static const u8 lcd[32] = {" + lcd + "};\n"
        "static const struct {u8 index,r,g,b;} ui[] = {" + overrides + "};\n",
        encoding="ascii")


def compile_host_display_palette(values):
    """Run the real C palette installer with synthetic 224-entry input."""
    palette_values = ",".join(f"0x{value:04x}" for value in values[32:])
    source = (
        '#include <stdio.h>\n'
        '#include "common/gba.c"\n'
        '#include "starcraft/sc64_display.h"\n'
        f'static const u16 input_palette[224] = {{{palette_values}}};\n'
        'int main(void) { gba_init(); sc64_display_palette(input_palette); '
        'for (int i=0;i<256;i++) printf("%u\\n", (unsigned)host_palette[i]); '
        'return 0; }\n'
    )
    with tempfile.TemporaryDirectory() as directory:
        c_file = Path(directory) / "palette_oracle.c"
        exe = Path(directory) / "palette_oracle"
        c_file.write_text(source, encoding="ascii")
        built = subprocess.run(
            ["cc", "-std=c99", "-Wall", "-Werror", "-DGBA_HOST",
             "-I", str(ROOT), str(c_file), "-o", str(exe)],
            capture_output=True, text=True)
        if built.returncode:
            raise AssertionError(f"host palette oracle compile failed:\n{built.stderr}")
        result = subprocess.run([str(exe)], capture_output=True, text=True)
        if result.returncode:
            raise AssertionError(f"host palette oracle failed:\n{result.stderr}")
        packed = [int(value) for value in result.stdout.splitlines()]
        if len(packed) != 256:
            raise AssertionError(f"host palette oracle emitted {len(packed)} colors")
        return [(value & 31, (value >> 5) & 31, (value >> 10) & 31)
                for value in packed]

class ImportTests(unittest.TestCase):
    def test_actual_palette_parser_matches_host_c_installer(self):
        values = synthetic_gba_palette()
        parsed, hashes = mod._actual_gba_palette(values)
        installed = compile_host_display_palette(values)
        self.assertEqual(parsed, installed)
        self.assertEqual(len(parsed), 256)
        self.assertEqual(set(hashes), {
            "common/gba.c", "common/gba.h", "starcraft/sc64_display.h"})
        self.assertTrue(all(len(value) == 64 for value in hashes.values()))

    def test_palette_parser_rejects_unrecognized_initializer_tokens(self):
        values = synthetic_gba_palette()
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            write_synthetic_palette_sources(root, malformed=True)
            with patch.object(mod, "ROOT", root), self.assertRaisesRegex(
                    ValueError, "unsupported UI colors initializer"):
                mod._actual_gba_palette(values)

    def test_modifier_lut_uses_all_source_and_actual_gba_colors(self):
        values = synthetic_gba_palette()
        source_colors = []
        for i in range(256):
            r = (i * 3 + 1) & 31
            g = (i * 9 + 7) & 31
            b = (i * 15 + 11) & 31
            source_colors.append((r, g, b))
        source_palette = b"".join(
            struct.pack(">H", (r << 11) | (g << 6) | (b << 1))
            for r, g, b in source_colors)
        bfire = bytes((row * 17 + column * 29 + 3) & 255
                       for row in range(40) for column in range(256))
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            write_synthetic_palette_sources(root)
            with patch.object(mod, "ROOT", root):
                actual_palette, _ = mod._actual_gba_palette(values)
                got = mod.warp_anchor_modifier_lut(source_palette, bfire, values)

        def distance(a, b):
            return sum((a[channel] - b[channel]) ** 2 for channel in range(3))

        source_for_destination = [
            min(range(256), key=lambda s: (distance(source_colors[s], color), s))
            for color in actual_palette]
        gba_for_source = [
            min(range(256), key=lambda d: (distance(source_colors[s], actual_palette[d]), d))
            for s in range(256)]
        expected = bytes(
            gba_for_source[bfire[row * 256 + source_for_destination[destination]]]
            for row in range(40) for destination in range(256))
        self.assertEqual(got, expected)
        self.assertEqual(len(got), 40 * 256)

        # A remap-preimage-only inverse would force each destination to its
        # identity source index here. Confirm that the synthetic palette has
        # cases where that shortcut produces a different modifier lookup.
        shortcut_differs = sum(
            gba_for_source[bfire[row * 256 + destination]] != got[row * 256 + destination]
            for row in range(40) for destination in range(256))
        self.assertGreater(shortcut_differs, 0)

    def test_modifier_lut_rejects_bad_table_dimensions(self):
        with self.assertRaisesRegex(ValueError, "invalid authenticated"):
            mod.warp_anchor_modifier_lut(bytes(512), bytes(40 * 256 - 1), [0] * 256)
        with self.assertRaisesRegex(ValueError, "invalid authenticated"):
            mod.warp_anchor_modifier_lut(bytes(511), bytes(40 * 256), [0] * 256)
        with self.assertRaisesRegex(ValueError, "invalid authenticated"):
            mod.warp_anchor_modifier_lut(bytes(512), bytes(40 * 256), [0] * 255)

    def test_modifier_writer_rejects_missing_frames_lut_and_bad_strengths(self):
        good_frame = {"x": 0, "y": 0, "width": 1, "height": 1,
                      "frame_width": 1, "frame_height": 1,
                      "pixels": bytes([40]), "alpha": bytes([1])}
        def write(path, frames, lut):
            mod.write_header(path, [{"image_id": mod.PROTOSS_WARP_ANCHOR_IMAGE,
                                     "frames": frames}], bytes(256),
                             modifier_lut=lut)
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "generated.h"
            with self.assertRaisesRegex(ValueError, "14 source frames"):
                write(path, [good_frame] * 13, bytes(40 * 256))
            with self.assertRaisesRegex(ValueError, "modifier LUT"):
                write(path, [good_frame] * 14, bytes(1))
            for invalid in (0, 41, 255):
                bad = {**good_frame, "pixels": bytes([invalid])}
                with self.subTest(strength=invalid), self.assertRaisesRegex(
                        ValueError, "invalid opaque strength"):
                    write(path, [bad] + [good_frame] * 13, bytes(40 * 256))

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
        struct.pack_into("<I", images, mod.PROTOSS_WARP_ANCHOR_IMAGE * 4, 215)
        tables = mod.table_images(bytes(units), bytes(flingy), bytes(sprites), bytes(images))
        payload = struct.pack("<IhH", 4, -1, 0)
        found = mod.select_images(lambda path: payload, tables)
        self.assertIn(325, found)
        self.assertEqual(found[325]["grp_path"], "00D/00C")
        self.assertIn("construction_image", found[325]["roles"])
        self.assertIn("normal_structure", found[275]["roles"])
        self.assertIn("warp_anchor", found[mod.PROTOSS_WARP_ANCHOR_IMAGE]["roles"])
        self.assertEqual(found[mod.PROTOSS_WARP_ANCHOR_IMAGE]["grp_path"], "00D/0D6")

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

    def test_protoss_stage_uses_shared_warp_anchor_init(self):
        units = bytearray(228 * 14)
        flingy = bytearray(209 * 2)
        sprites = bytearray(517 * 2)
        images = bytearray(999 * 38)
        # The normal structure has its own WarpIn, but OpenBW first replaces
        # every Protoss construction sprite with the shared Warp Anchor image.
        struct.pack_into("<I", images,
                         999 * 10 + mod.PROTOSS_WARP_ANCHOR_IMAGE * 4, 1)
        tables = mod.table_images(bytes(units), bytes(flingy), bytes(sprites), bytes(images))

        # Script id 1 has Init; both source building IDs point to that shared
        # anchor program, independent of their normal structure images.
        payload = bytearray(72)
        struct.pack_into("<IhHhH", payload, 0, 4, 1, 16, -1, 0)
        payload[16:20] = b"SCPE"
        struct.pack_into("<I", payload, 20, 15)
        struct.pack_into("<H", payload, 24, 64)
        payload[64] = 22
        stages = mod.source_stage_inputs(tables, bytes(payload))
        got = {(item["raw_unit_id"], item["image_id"], item["animation"])
               for item in stages}
        protoss_buildings = {raw for race, raw in mod.BUILDINGS.values()
                             if race == "protoss"}
        self.assertEqual(got, {(raw, mod.PROTOSS_WARP_ANCHOR_IMAGE, "Init")
                               for raw in protoss_buildings})

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
