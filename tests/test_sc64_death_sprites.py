#!/usr/bin/env python3
"""Asset-free tests for the death-animation layer compiler and header writer."""
from __future__ import annotations

import shutil
import struct
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
from tools import sc64_death_sprites as mod  # noqa: E402


def build_payload(scripts: dict[int, dict[int, bytes]]) -> bytes:
    """Assemble an IScript payload: {script_id: {animation_id: code}}."""
    payload = bytearray(4)
    records = {}
    for script_id, animations in scripts.items():
        highest = max(animations)
        count = (highest + 2) & ~1
        record = len(payload)
        payload += b"SCPE" + struct.pack("<I", highest) + bytes(count * 2)
        records[script_id] = record
        for animation_id, code in animations.items():
            struct.pack_into("<H", payload, record + 8 + animation_id * 2, len(payload))
            payload += code
    table = len(payload)
    for script_id, record in records.items():
        payload += struct.pack("<hH", script_id, record)
    payload += struct.pack("<hH", -1, 0)
    struct.pack_into("<I", payload, 0, table)
    return bytes(payload)


def op(code: int, *args: tuple[str, int]) -> bytes:
    out = bytes([code])
    for fmt, value in args:
        out += struct.pack("<" + fmt, value)
    return out


class CompileTests(unittest.TestCase):
    def setUp(self):
        self.image_iscript = [0] * 999
        self.sprite_image = [0] * 517
        self.image_iscript[10] = 1
        self.image_iscript[20] = 2
        self.image_iscript[30] = 3
        self.sprite_image[5] = 30

    def compile(self, death: bytes, extra: dict[int, dict[int, bytes]] | None = None):
        scripts = {1: {1: death}}
        scripts.update(extra or {})
        payload = build_payload(scripts)
        return mod.compile_death(payload, mod.cs.script_index(payload),
                                 self.image_iscript, self.sprite_image, 10)

    def test_collapse_overlay_and_capped_looping_remnant(self):
        # Death: two collapse frames, then an explosion overlay and a remnant.
        death = (op(0, ("H", 3)) + op(5, ("B", 2)) + op(0, ("H", 4)) + op(5, ("B", 2)) +
                 op(8, ("H", 20), ("b", 1), ("b", -2)) + op(17, ("H", 5), ("B", 0), ("B", 0)) +
                 op(5, ("B", 1)) + op(24, ("H", 7)) + op(22))
        overlay = op(0, ("H", 0)) + op(5, ("B", 2)) + op(0, ("H", 1)) + op(5, ("B", 2)) + op(22)
        # Remnant repeats one frame with long waits through a goto loop.
        remnant = op(0, ("H", 0)) + op(5, ("B", 125)) + op(7, ("H", 0))
        payload_scripts = {2: {0: overlay}, 3: {0: remnant}}
        # The goto target must be the remnant's absolute start; patch below.
        payload = build_payload({1: {1: death}, **payload_scripts})
        start = mod.cs.image_animation_starts(payload, mod.cs.script_index(payload)[3])["Init"]
        remnant = op(0, ("H", 0)) + op(5, ("B", 125)) + op(7, ("H", start))
        layers = self.compile(death, {2: {0: overlay}, 3: {0: remnant}})
        self.assertEqual([l["image_id"] for l in layers], [10, 20, 30])
        actor, explosion, rest = layers
        self.assertEqual(actor["steps"], [[3, 3], [4, 5]])
        self.assertEqual((actor["start"], actor["end"], actor["under"]), (0, 8, True))
        self.assertEqual(explosion["steps"], [[0, 3], [1, 3]])
        self.assertEqual((explosion["start"], explosion["end"], explosion["under"]), (6, 12, False))
        self.assertEqual((explosion["x"], explosion["y"]), (1, -2))
        self.assertTrue(rest["remnant"] and rest["under"])
        self.assertEqual(rest["loop_start"], 0)
        self.assertEqual((rest["start"], rest["end"]), (6, 6 + mod.REMNANT_CAP_SU))
        self.assertEqual(mod.timing(layers),
                         {"burst_su": 12, "remnant_start": 6, "total_su": 6 + mod.REMNANT_CAP_SU})

    def test_unsupported_and_degenerate_scripts_fail_closed(self):
        cases = (
            op(0, ("H", 1)) + op(30, ("B", 1), ("H", 0)),  # randcondjmp
            op(5, ("B", 1)) + op(22),                         # no frames drawn
            op(8, ("H", 21), ("B", 0), ("B", 0)) + op(22),    # overlay without IScript
        )
        for death in cases:
            with self.subTest(death=death), self.assertRaises(mod.Unsupported):
                self.compile(death)
        # A loop without a wait never advances.
        payload = build_payload({1: {1: op(0, ("H", 1)) + op(7, ("H", 0))}})
        start = mod.cs.image_animation_starts(payload, mod.cs.script_index(payload)[1])["Death"]
        with self.assertRaises(mod.Unsupported):
            self.compile(op(0, ("H", 1)) + op(7, ("H", start + 3)))

    def test_lift_off_branch_follows_grounded_path(self):
        death = op(0, ("H", 2)) + op(63, ("H", 0)) + op(5, ("B", 3)) + op(22)
        layers = self.compile(death)
        self.assertEqual(layers[0]["steps"], [[2, 4]])


class PixelTests(unittest.TestCase):
    def test_ui_palette_and_ramps(self):
        ui = mod.ui_palette_indices()
        self.assertEqual((ui["C_BLACK"], ui["C_WHITE"], ui["C_ORANGE"], ui["C_HIGHLIGHT"]),
                         (0, 1, 15, 31))
        self.assertEqual(mod.ramp_indices(4, ui)[-1], ui["C_WHITE"])
        self.assertEqual(mod.ramp_indices(99, ui), mod.ramp_indices(1, ui))
        self.assertEqual([mod.intensity_band(v) for v in (1, 8, 16, 26, 36, 44, 60)],
                         [0, 1, 2, 3, 4, 5, 4])

    def test_remapped_frames_drop_faint_band_and_dither(self):
        width = 4
        values = [1, 10, 10, 47]
        frame = {"x": 0, "y": 0, "frame_width": width, "frame_height": 1, "width": width,
                 "height": 1, "pixels": bytes(values), "alpha": bytes([1] * width)}
        ramp = list(range(100, 106))
        out = mod.convert_frames({"frames": [frame]}, [0], 9, 1, bytes(256), ramp)[0]
        self.assertEqual(list(out["pixels"]), [0, 0, 101, 105])  # odd column dithered
        remap = bytes((i + 1) % 256 for i in range(256))
        plain = mod.convert_frames({"frames": [frame]}, [0], 0, 0, remap, [])[0]
        self.assertEqual(list(plain["pixels"]), [2, 11, 11, 48])


class HeaderTests(unittest.TestCase):
    def test_header_compiles_and_dedupes_programs(self):
        layer = {"image_id": 7, "x": 0, "y": -1, "start": 0, "end": 6, "under": False,
                 "remnant": False, "steps": [[5, 3], [9, 3]], "loop_start": mod.HOLD_LAST}
        sources = {raw: {"layers": [dict(layer)], "timing": mod.timing([layer])} for raw in (2, 3)}
        images = {7: {"map": {5: 0, 9: 1}, "frames": [
            {"x": 1, "y": 2, "w": 2, "h": 1, "canvas_w": 8, "canvas_h": 8, "pixels": b"\x01\x02"},
            {"x": 0, "y": 0, "w": 1, "h": 1, "canvas_w": 8, "canvas_h": 8, "pixels": b"\x03"}]}}
        with tempfile.TemporaryDirectory() as tmp:
            header = Path(tmp) / "sc64_death_sprites_generated.h"
            mod.write_header(header, sources, images)
            text = header.read_text()
            self.assertEqual(text.count("SC64ConstructionAnimStep sc64_death_program_"), 1)
            self.assertEqual(text.count("Sc64DeathSourceLayer sc64_death_layers_"), 1)
            self.assertIn("{ 0u, 3u },", text)
            self.assertIn("#define SC64_DEATH_SOURCE_ACTOR_COUNT 2u", text)
            clang = shutil.which("clang")
            if clang:
                probe = Path(tmp) / "probe.c"
                probe.write_text(
                    "typedef struct { short offset_x, offset_y; unsigned short w, h, canvas_w, "
                    "canvas_h; const unsigned char *pixels; } Sc64SpriteFrame;\n"
                    '#include "sc64_death_sprites_generated.h"\n'
                    "int main(void) { return !sc64_death_source_valid(sc64_death_source(3)) || "
                    "!sc64_death_image_frame(7, 1) || sc64_death_image_frame(7, 2) != 0; }\n")
                exe = Path(tmp) / "probe"
                subprocess.run([clang, "-std=c11", "-Wall", "-Wextra", "-Werror",
                                "-Wno-unused-function", "-DSC64_DEATH_SOURCE_ART",
                                f"-I{tmp}", f"-I{ROOT / 'starcraft'}", str(probe), "-o", str(exe)],
                               check=True)
                subprocess.run([str(exe)], check=True)


if __name__ == "__main__":
    unittest.main()
