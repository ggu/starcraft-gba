import unittest
import shutil
import subprocess
import tempfile
from pathlib import Path

from tools import sc64_unit_attack_sprites as attack


class AttackPoseSourceTests(unittest.TestCase):
    def test_profile_audit_actor_ids_are_explicit_and_turrets_are_not_body_poses(self):
        expected = {
            "marine": 0, "ghost": 1, "vulture": 2, "scv": 7, "wraith": 8,
            "battlecruiser": 12, "kerrigan": 16, "raynor_vulture": 19,
            "raynor_marine": 20, "norad_ii": 29, "firebat": 32,
            "zergling": 37, "hydralisk": 38, "ultralisk": 39, "drone": 41,
            "mutalisk": 43, "guardian": 44, "scourge": 47,
            "infested_terran": 50, "infested_kerrigan": 51,
            "hunter_killer": 53, "probe": 64, "zealot": 65, "dragoon": 66,
            "archon": 68, "scout": 70, "arbiter": 71, "interceptor": 73,
            "dark_templar": 74, "zeratul": 75, "fenix": 77,
            "fenix_dragoon": 78, "tassadar": 79, "reaver": 83,
        }
        self.assertEqual(attack.ATTACK_UNITS, expected)
        self.assertNotIn(3, attack.ATTACK_UNITS.values())
        self.assertNotIn(5, attack.ATTACK_UNITS.values())
        self.assertNotIn(30, attack.ATTACK_UNITS.values())

    def test_source_wait_timing_and_release_event(self):
        # playframe(17), wait(3), attack (opcode 38), wait(2),
        # playframe(34), wait(1), end.
        script = bytes((0, 17, 0,  5, 3,  38,  5, 2,  0, 34, 0,  5, 1,  22))
        result = attack.walk_animation(script, 0, set())
        self.assertEqual(result["releases"], [{"tick": 3, "opcode": 38, "kind": "attack"}])
        self.assertEqual(attack.sample_events(result["frames"], 0, 0), 17)
        self.assertEqual(attack.sample_events(result["frames"], 2, 0), 17)
        self.assertEqual(attack.sample_events(result["frames"], 3, 0), 17)
        self.assertEqual(attack.sample_events(result["frames"], 4, 0), 17)
        self.assertEqual(attack.sample_events(result["frames"], 5, 0), 34)
        self.assertEqual(result["end_tick"], 6)
        self.assertEqual(attack.sample_events(result["frames"], 0, 0), 17)

    def test_release_at_tick_zero_and_animation_boundary(self):
        script = bytes((38, 5, 2, 0, 51, 0, 22))
        result = attack.walk_animation(script, 0, set())
        self.assertEqual(result["releases"][0]["tick"], 0)
        self.assertEqual(attack.sample_events(result["frames"], 0, 0), 0)
        self.assertEqual(attack.sample_events(result["frames"], 2, 0), 51)

        # A goto into the next named animation is a boundary, not a loop.
        script = bytes((37, 1, 7, 7, 0, 1, 0, 22))
        result = attack.walk_animation(script, 0, {7})
        self.assertEqual(len(result["releases"]), 1)
        self.assertEqual(result["frames"][-1], {"tick": 0, "frame": 0})

    def test_heading_to_source_directional_frame(self):
        self.assertEqual([attack.resolve_frame_index(34, h, True, 51)
                          for h in (0, 16, 17, 31)], [34, 50, 49, 35])
        self.assertEqual(attack.resolve_frame_index(7, 31, False, 8), 7)
        with self.assertRaises(ValueError):
            attack.resolve_frame_index(35, 0, True, 51)
        with self.assertRaises(ValueError):
            attack.resolve_frame_index(0, 32, True, 51)

    def test_mirror_preserves_canvas_crop_and_pixels(self):
        frame = {"width": 4, "height": 2, "x": 1, "y": 0,
                 "frame_width": 2, "frame_height": 1,
                 "pixels": bytes((1, 2, 3, 4, 5, 6, 7, 8)),
                 "alpha": bytes((1, 0, 1, 1, 0, 1, 0, 1))}
        mirrored = attack.orient_source_frame(frame, True)
        self.assertEqual(mirrored["width"], 4)
        self.assertEqual(mirrored["height"], 2)
        self.assertEqual(mirrored["x"], 1)
        self.assertEqual(mirrored["pixels"], bytes((4, 3, 2, 1, 8, 7, 6, 5)))
        self.assertEqual(mirrored["alpha"], bytes((1, 1, 0, 1, 1, 0, 1, 0)))

    def test_crop_remap_keeps_transparency(self):
        frame = {"width": 3, "height": 2, "x": 1, "y": 0,
                 "frame_width": 2, "frame_height": 2,
                 "pixels": bytes((9, 10, 11, 12, 13, 14)),
                 "alpha": bytes((1, 1, 0, 0, 1, 1))}
        remap = bytes((255 - i) & 0xff for i in range(256))
        mapped = attack.map_frame(frame, remap)
        self.assertEqual(mapped["pixels"], bytes((245, 0, 242, 241)))
        self.assertEqual((mapped["offset_x"], mapped["offset_y"],
                          mapped["w"], mapped["h"],
                          mapped["canvas_w"], mapped["canvas_h"]), (1, 0, 2, 2, 3, 2))
        with self.assertRaises(ValueError):
            attack.map_frame(frame, b"short")

    def test_init_composite_keeps_body_and_both_attached_layers(self):
        def solid(value, width=2, height=2):
            return {"width": width, "height": height, "x": 0, "y": 0,
                    "frame_width": width, "frame_height": height,
                    "pixels": bytes([value] * (width * height)),
                    "alpha": bytes([1] * (width * height))}

        body = solid(1)
        body["alpha"] = bytes((1, 1, 0, 1))
        shadow = solid(3, 1, 1)
        attack_body = solid(2, 1, 1)
        composite = attack.races.compose_init_layers(
            body, [(shadow, 0, 1, True), (attack_body, 1, 0, False)])
        x, y, width = composite["x"], composite["y"], composite["width"]
        pixels = composite["pixels"]
        self.assertEqual((x, y, width, composite["height"]), (0, 0, 2, 2))
        self.assertEqual(pixels, bytes((1, 2, 3, 1)))
        self.assertEqual(composite["alpha"], bytes((1, 1, 1, 1)))

    def test_header_keeps_ground_and_air_modes_in_one_switch_case(self):
        step = {"tick": 0, "frames": [{
            "offset_x": 0, "offset_y": 0, "w": 1, "h": 1,
            "canvas_w": 1, "canvas_h": 1, "pixels": b"\x01"} for _ in range(32)]}
        compiled = {(66, 0): {"release_tick": 2, "steps": [step]},
                    (66, 1): {"release_tick": 4, "steps": [step]}}
        from pathlib import Path
        import tempfile
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "generated.h"
            attack.write_header(path, compiled)
            source = path.read_text()
        self.assertEqual(source.count("case 66u:"), 5)  # release, two durations, two samplers.
        self.assertIn("if (!!target_air == 0) return 2u;", source)
        self.assertIn("if (!!target_air == 1) return 4u;", source)
        self.assertIn("if (!!target_air == 0) return 1u;", source)
        self.assertIn("if (!!target_air == 1) return 1u;", source)
        self.assertIn("sc64_unit_attack_source_duration", source)
        self.assertIn("sc64_unit_attack_source_full_duration", source)
        self.assertIn("sc64_unit_attack_source_full_frame", source)
        self.assertIn("return 3u;", source)  # compatibility fixture's full source span.
        self.assertIn("SC64_UNIT_ATTACK_POSE_PIXEL_BYTES 1u", source)

    def test_full_timeline_holds_last_pose_until_terminal_wait_boundary(self):
        # A frame set at tick 0 followed by wait(5), end is visible at source
        # ticks 0..4. The script's terminal boundary is tick 5.
        script = bytes((0, 0, 0, 5, 5, 22))
        walked = attack.walk_animation(script, 0, set())
        self.assertEqual(walked["end_tick"], 5)
        self.assertEqual(walked["frames"], [{"tick": 0, "frame": 0},
                                            {"tick": 0, "frame": 0}])
        frame = {"offset_x": 0, "offset_y": 0, "w": 1, "h": 1,
                 "canvas_w": 1, "canvas_h": 1, "pixels": b"\x01"}
        step = {"tick": 0, "frames": [frame] * 32}
        compiled = {(66, 0): {"release_tick": 0, "steps": [step],
                             "full_steps": [step],
                             "full_duration_source_ticks": walked["end_tick"]}}
        compiler = shutil.which("clang") or shutil.which("cc")
        self.assertIsNotNone(compiler, "a C compiler is required for the generated-header oracle")
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            header = root / "generated.h"
            attack.write_header(header, compiled)
            source = root / "oracle.c"
            source.write_text(
                '#include "generated.h"\n'
                '#include <assert.h>\n'
                'int main(void) {\n'
                ' const Sc64SpriteFrame *first = sc64_unit_attack_source_full_frame(66,0,0,0);\n'
                ' assert(first != 0);\n'
                ' for (unsigned t=1;t<5;++t) assert(sc64_unit_attack_source_full_frame(66,0,0,t)==first);\n'
                ' assert(sc64_unit_attack_source_full_duration(66,0)==5);\n'
                ' assert(sc64_unit_attack_source_full_frame(66,0,0,5)==0);\n'
                ' return 0;\n}\n', encoding="ascii")
            executable = root / "oracle"
            subprocess.run([compiler, "-std=c11", "-Wall", "-Wextra", "-Werror",
                            "-I", str(root), str(source), "-o", str(executable)],
                           check=True, capture_output=True, text=True)
            subprocess.run([str(executable)], check=True, capture_output=True, text=True)


if __name__ == "__main__":
    unittest.main()
