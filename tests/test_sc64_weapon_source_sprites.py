#!/usr/bin/env python3
"""Asset-free checks for source-weapon sprite conversion primitives."""
# SPDX-License-Identifier: GPL-3.0-or-later
from __future__ import annotations

import importlib.util
import struct
import unittest
from tools.sc64_construction_sprites import decode_instruction
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "sc64_weapon_source_sprites", ROOT / "tools/sc64_weapon_source_sprites.py")
assert SPEC and SPEC.loader
source = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(source)
AUDIT_SPEC = importlib.util.spec_from_file_location(
    "sc64_weapon_source_audit", ROOT / "tools/sc64_weapon_source_audit.py")
assert AUDIT_SPEC and AUDIT_SPEC.loader
audit = importlib.util.module_from_spec(AUDIT_SPEC)
AUDIT_SPEC.loader.exec_module(audit)


class WeaponSourceSpriteTests(unittest.TestCase):
    def test_iscript_signal_ordinals_timing_and_call_boundary(self):
        # playframe 0; wait 1; setspawnframe 7; sigorder 1; wait 3;
        # setfldirect 4; call 0x1234.  A call ends this bounded report.
        data = bytes([0, 0, 0, 5, 1, 35, 7, 36, 1, 5, 3, 52, 4, 53, 0x34, 0x12, 22])
        events, tick, loop, complete = audit.trace_iscript(data, 0, decode_instruction)
        self.assertTrue(complete)
        self.assertIsNone(loop)
        self.assertEqual(tick, 4)
        self.assertEqual([(e["pc"], e["opcode"], e["name"], e["args"], e["source_tick"])
                          for e in events], [
            (0, 0, "playframe", [0], 0), (3, 5, "wait", [1], 0),
            (5, 35, "setspawnframe", [7], 1), (7, 36, "sigorder", [1], 1),
            (9, 5, "wait", [3], 1), (11, 52, "setfldirect", [4], 4),
            (13, 53, "call", [0x1234], 4)])
        with self.assertRaisesRegex(ValueError, "truncated IScript operand"):
            audit.trace_iscript(bytes([36]), 0, decode_instruction)
        with self.assertRaisesRegex(ValueError, "truncated IScript branch operand"):
            audit.trace_iscript(bytes([53, 0x34]), 0, decode_instruction)

    def test_normal_palette_remap_preserves_transparency_and_opaque(self):
        remap = bytearray(range(256))
        remap[0] = 1
        self.assertEqual(source.convert_pixels(bytes([0, 17, 255]), bytes([0, 1, 1]), 0, remap),
                         bytes([0, 17, 255]))
        with self.assertRaises(ValueError):
            source.convert_pixels(bytes([2]), bytes([1]), 1, bytes(256))

    def test_modifier9_uses_native_selector_values_and_all_strengths(self):
        vals = bytes(range(1, 64))
        self.assertEqual(source.convert_pixels(vals, bytes([1]) * 63, 9, bytes(256)), vals)
        self.assertEqual(source.convert_pixels(bytes([0]), bytes([0]), 9, bytes(256)), b"\0")
        for value in (0, 64, 255):
            with self.assertRaises(ValueError):
                source.convert_pixels(bytes([value]), bytes([1]), 9, bytes(256))

    def test_modifier10_is_an_alpha_selector(self):
        self.assertEqual(source.convert_pixels(bytes([0, 2, 4]), bytes([0, 1, 1]), 10, bytes(256)),
                         bytes([0, 1, 1]))

    def test_modifier9_source_shift_table_bindings(self):
        self.assertEqual([source.modifier9_table_path(i) for i in range(1, 5)],
                         ["00B/006", "00B/007", "00B/008", "00B/009"])
        with self.assertRaises(ValueError):
            source.modifier9_table_path(0)

    def test_lut_projection_uses_raw_rgba5551_palette(self):
        palette_words = [((i % 32) << 11) | (((i * 7) % 32) << 6) |
                         (((i * 13) % 32) << 1) | 1 for i in range(256)]
        source_palette = struct.pack(">256H", *palette_words)
        table = bytes((row * 3 + dest * 5) & 0xFF
                      for row in range(40) for dest in range(256))
        display_palette = [((i * 211) ^ (i << 3)) & 0x7FFF for i in range(256)]
        palette_json = {"source_rgb555": [0] * 256,
                        "gba_rgb555": display_palette}
        def get(path):
            return {"00B/004": source_palette, "00B/008": table}[path]
        luts, projected, _ = source._project_luts(
            get, palette_json, {"211": {"modifier": 9, "color_shift": 3}})
        helper = source._load("sc64_weapon_source_effect_palette_test",
                              ROOT / "tools/sc64_effect_palette.py")
        expected = helper.project_light_table(source_palette, table, display_palette)
        self.assertEqual(luts[0]["bolt_path"], "00B/008")
        self.assertEqual(projected, expected)

    def test_direction_fold_and_mirror_are_exact(self):
        for heading in range(32):
            frame, mirror = source.direction_frame(34, 17, heading)
            folded = heading if heading <= 16 else 32 - heading
            self.assertEqual(frame, 34 + folded)
            self.assertEqual(mirror, heading > 16)
        with self.assertRaises(ValueError):
            source.direction_frame(0, 16, 16)
        self.assertEqual(source.mirror_crop(3, 7, 24), 14)
        with self.assertRaises(ValueError):
            source.mirror_crop(20, 8, 24)

    def test_profile_binds_owner_animation_projectile_and_source_tick(self):
        fields = {"bullet_type": 0, "count": 2, "flingy": 7, "lifetime": 12,
                  "forward": 3, "upward": -2}
        report = {
            "referenced_images": {"200": {"image_id": 200}},
            "units": [{"raw_unit": 0,
                       "actor_images": [{"raw_unit": 0, "image": 100}],
                       "weapons": [{"mode": "ground", "weapon_id": 9,
                                    "weapon_owner_raw": 0, "weapon_fields": fields,
                                    "weapon_graphic_chain": {"image": 200,
                                        "flingy_motion": {"top_speed_raw": 12,
                                            "acceleration_raw": 3,"halt_distance_raw": 4}}}]}],
            "buildings": []}
        animations = [
            {"image_id": 100, "animation_id": 2, "event_first": 0, "event_count": 1,
             "overlay_first": 0, "overlay_count": 1},
            {"image_id": 200, "animation_id": 0, "event_first": 1, "event_count": 2,
             "overlay_first": 1, "overlay_count": 0}]
        events = [{"source_tick": 7, "opcode": 37, "source_frame": 0xFFFF},
                  {"source_tick": 0, "opcode": 0, "source_frame": 4},
                  {"source_tick": 2, "opcode": 0, "source_frame": 5}]
        rows = source._profile_rows(report, animations, events, [{}])
        self.assertEqual(len(rows), 1)
        row = rows[0]
        self.assertEqual((row["actor_raw"], row["weapon_owner_raw"], row["weapon_id"], row["mode"]),
                         (0, 0, 9, 0))
        self.assertEqual((row["attack_image"], row["projectile_image"], row["attackwith_tick"]),
                         (100, 200, 7))
        self.assertEqual((row["attack_event_first"], row["attack_event_count"],
                          row["projectile_event_first"], row["projectile_event_count"]),
                         (0, 1, 1, 2))
        self.assertEqual((row["projectile_first_frame"], row["projectile_frame_count"],
                          row["projectile_ticks_per_frame"]), (4, 2, 2))
        self.assertEqual(row["flags"] & 1, 1)
        self.assertEqual((row["speed_raw"], row["accel_raw"], row["halt_raw"]), (12, 3, 4))

    def test_profile_follows_attached_archon_weapon_image(self):
        fields = {"bullet_type": 2, "count": 1, "flingy": 7, "lifetime": 12,
                  "forward": 0, "upward": 0}
        report = {"referenced_images": {"200": {"image_id": 200},
                                        "135": {"image_id": 135}},
            "source_animations": {
                "134": {"Init": {"visual_references": [{"image_id": 135}]}},
                "135": {"GndAttkInit": {"status": "ok", "events": [
                    {"opcode": 38, "source_tick": 5}],
                    "visual_references": [{"image_id": 549}]}}},
            "units": [{"raw_unit": 68, "actor_images": [{"raw_unit": 68, "image": 134}],
                "weapons": [{"mode": "ground", "weapon_id": 70,
                    "weapon_owner_raw": 68, "weapon_fields": fields,
                    "weapon_graphic_chain": {"image": 200}}]}], "buildings": []}
        animations = [{"image_id": 135, "animation_id": 2, "event_first": 0,
            "event_count": 1, "overlay_first": 0, "overlay_count": 1}]
        events = [{"source_tick": 5, "opcode": 38, "source_frame": 0xFFFF}]
        overlays = [{"parent_image": 135, "image_id": 549, "source_tick": 4}]
        row = source._profile_rows(report, animations, events, overlays)[0]
        self.assertEqual(row["attack_image"], 135)
        self.assertEqual(row["attackwith_tick"], 5)
        self.assertEqual((row["attack_event_first"], row["attack_event_count"],
                          row["overlay_first"], row["overlay_count"]), (0, 1, 0, 1))

    def test_used_directional_frame_expands_its_17_frame_bank(self):
        report = {"source_animations": {"11": {"Init": {"status": "ok", "events": [
            {"name": "playframe", "args": [34]}]}}},
            "referenced_images": {"11": {"directional": True, "frame_count": 85}}}
        self.assertEqual(source._used_frames(report)[11], set(range(34, 51)))

    def test_lo_table_resolves_native_special_category_by_parent_frame(self):
        image_count = 999
        script_end = 4 * image_count + 6 * image_count + 4 * image_count
        image_data = bytearray(script_end + 6 * 4 * image_count)
        special_column = script_end + 3 * 4 * image_count
        struct.pack_into("<I", image_data, special_column + 254 * 4, 261)
        raw = bytearray(32)
        struct.pack_into("<II", raw, 0, 3, 2)
        for frame, pos in enumerate((20, 24, 28)):
            struct.pack_into("<I", raw, 8 + frame * 4, pos)
        struct.pack_into("<bbbbbb", raw, 20, 1, 2, -3, 4, 5, -6)
        tables, offsets, hashes = source._collect_lo_tables(
            lambda path: raw if path == "00E/004" else (_ for _ in ()).throw(KeyError(path)),
            bytes(image_data), {"referenced_images": {"254": {"image_id": 254}},
                "source_animations": {"254": {"GndAttkInit": {"visual_references": [
                    {"opcode":13,"args":[537,2,0]}]}}}})
        self.assertEqual(tables, [{"image_id": 254, "lo_kind": 2,
            "frame_count": 3, "offset_count": 2, "offset_first": 0,
            "gfi": 261, "bolt_path": "00E/004"}])
        self.assertEqual(offsets, [{"x":1,"y":2},{"x":-3,"y":4},
                                   {"x":5,"y":-6},{"x":0,"y":0},
                                   {"x":0,"y":0},{"x":0,"y":0}])
        self.assertIn("00E/004", hashes)

    def test_raster_bank_keeps_projectiles_and_child_layers_only(self):
        report = {"units": [{"actor_images": [{"image":10}],
                      "weapons": [{"weapon_graphic_chain": {"image": 30}}]}],
                  "buildings": [],
                  "source_animations": {"10": {"GndAttkInit": {"visual_references": [
                      {"image_id": 40}, {"image_id": -1}]}}}}
        self.assertEqual(source._runtime_images(report), {30, 40})

    def test_special_emitter_adds_spider_mine_without_claiming_actor_profile(self):
        actors = {7: ["U_SCV"]}
        emission_only = audit.add_special_emission_actors(actors)
        self.assertEqual(actors[13], ["Spider Mine (detonation emitter)"])
        self.assertEqual(emission_only, {13: "Spider Mine (detonation emitter)"})
        self.assertNotIn("Spider Mine (detonation emitter)", actors[7])

    def test_spider_mine_weapon6_becomes_source_profile(self):
        fields = {"bullet_type": 5, "count": 1, "flingy": 172,
                  "lifetime": 255, "forward": 0, "upward": 0}
        report = {
            "referenced_images": {"500": {"image_id": 500}},
            "source_animations": {},
            "units": [{"raw_unit": 13, "actor_images": [{"raw_unit": 13, "image": 300}],
                       "weapons": [{"mode": "ground", "weapon_id": 6,
                                    "weapon_owner_raw": 13, "weapon_fields": fields,
                                    "weapon_graphic_chain": {"image": 500,
                                        "flingy_motion": {"top_speed_raw": 800,
                                            "acceleration_raw": 0, "halt_distance_raw": 0}}}]}],
            "buildings": []}
        row = source._profile_rows(report, [], [], [])[0]
        self.assertEqual((row["actor_raw"], row["weapon_owner_raw"],
                          row["weapon_id"], row["weapon_bullet_type"]), (13, 13, 6, 5))
        self.assertEqual((row["projectile_image"], row["projectile_flingy"],
                          row["projectile_lifetime"]), (500, 172, 255))

    def test_generated_lookup_contract_is_sorted_and_logarithmic(self):
        rows = [{"image_id": 8, "animation_id": 2},
                {"image_id": 3, "animation_id": 5},
                {"image_id": 3, "animation_id": 0}]
        self.assertEqual(source._sorted_animation_rows(rows), [
            {"image_id": 3, "animation_id": 0},
            {"image_id": 3, "animation_id": 5},
            {"image_id": 8, "animation_id": 2}])
        accessors = "\n".join(source._lookup_accessor_lines())
        for name in ("profile_lookup", "image_lookup", "animation_lookup"):
            self.assertIn(f"sc64_weapon_source_{name}", accessors)
        self.assertGreaterEqual(accessors.count("while(lo<hi)"), 4)

    def test_special_emitter_adds_spider_mine_without_claiming_actor_profile(self):
        actors = {7: ["U_SCV"]}
        emission_only = audit.add_special_emission_actors(actors)
        self.assertEqual(actors[13], ["Spider Mine (detonation emitter)"])
        self.assertEqual(emission_only, {13: "Spider Mine (detonation emitter)"})
        self.assertNotIn(13, {7: ["U_SCV"]})

    def test_spider_mine_weapon6_becomes_source_profile(self):
        fields = {"bullet_type": 5, "count": 1, "flingy": 172,
                  "lifetime": 255, "forward": 0, "upward": 0}
        report = {
            "referenced_images": {"500": {"image_id": 500}},
            "source_animations": {},
            "units": [{"raw_unit": 13, "actor_images": [{"raw_unit": 13, "image": 300}],
                       "weapons": [{"mode": "ground", "weapon_id": 6,
                                    "weapon_owner_raw": 13, "weapon_fields": fields,
                                    "weapon_graphic_chain": {"image": 500,
                                        "flingy_motion": {"top_speed_raw": 800,
                                            "acceleration_raw": 0, "halt_distance_raw": 0}}}]}],
            "buildings": []}
        row = source._profile_rows(report, [], [], [])[0]
        self.assertEqual((row["actor_raw"], row["weapon_owner_raw"],
                          row["weapon_id"], row["weapon_bullet_type"]), (13, 13, 6, 5))
        self.assertEqual((row["projectile_image"], row["projectile_flingy"],
                          row["projectile_lifetime"]), (500, 172, 255))


if __name__ == "__main__":
    unittest.main()
