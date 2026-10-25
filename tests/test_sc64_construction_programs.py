"""Synthetic-only tests for the bounded IScript compiler."""

from __future__ import annotations

import unittest

from tools.sc64_construction_programs import (
    HOLD_LAST,
    compile_primary_frame_stream,
    compile_source_layers,
)
from tools.sc64_construction_sprites import decode_instruction


def play(frame: int) -> bytes:
    return bytes((0, frame & 0xFF, frame >> 8))


def wait(ticks_minus_one: int) -> bytes:
    return bytes((5, ticks_minus_one))


def goto(target: int) -> bytes:
    return bytes((7, target & 0xFF, target >> 8))


class CompilePrimaryFrameStreamTests(unittest.TestCase):
    def compile(self, code: bytes, *, frame_count: int = 4,
                initial_frame: int = 0) -> dict:
        return compile_primary_frame_stream(code, 0, frame_count, initial_frame)

    def test_intro_then_loop_has_source_wait_durations(self) -> None:
        # frame 0 wait 1; loop: frame 1 wait 0; frame 2 wait 0; goto loop.
        loop_start = 8
        code = (play(0) + wait(1) + goto(loop_start) + play(1) + wait(0) +
                play(2) + wait(0) + goto(loop_start))
        result = self.compile(code, frame_count=3)
        self.assertEqual(result, {
            "steps": [
                {"frame": 0, "duration_source_updates": 2},
                {"frame": 1, "duration_source_updates": 1},
                {"frame": 2, "duration_source_updates": 1},
            ],
            "loop_start": 1,
            "frame_count": 3,
            "omitted_layers": [],
            "signals": [],
        })

    def test_wait_zero_is_one_source_update(self) -> None:
        result = self.compile(play(2) + wait(0) + bytes((22,)))
        self.assertEqual(result["steps"], [
            {"frame": 2, "duration_source_updates": 1}
        ])
        self.assertEqual(result["loop_start"], HOLD_LAST)

    def test_initial_frame_survives_without_playfram(self) -> None:
        result = self.compile(wait(0) + bytes((22,)), initial_frame=3)
        self.assertEqual(result["steps"], [
            {"frame": 3, "duration_source_updates": 1}
        ])

    def test_end_holds_a_new_frame_after_previous_wait(self) -> None:
        result = self.compile(wait(3) + play(2) + bytes((22,)))
        self.assertEqual(result["steps"], [
            {"frame": 0, "duration_source_updates": 4},
            {"frame": 2, "duration_source_updates": 1},
        ])

    def test_repeat_key_includes_current_frame(self) -> None:
        # Offset 3 first carries frame 0 and later frame 1. A PC-only detector
        # would falsely identify the second visit as the loop boundary.
        target = 3
        code = goto(target) + wait(0) + play(1) + wait(0) + goto(target)
        result = self.compile(code, frame_count=2)
        self.assertEqual(result["steps"], [
            {"frame": 0, "duration_source_updates": 1},
            {"frame": 1, "duration_source_updates": 1},
            {"frame": 1, "duration_source_updates": 1},
        ])
        self.assertEqual(result["loop_start"], 1)

    def test_primary_timeline_reports_omitted_layers_and_signals(self) -> None:
        code = bytes((
            8, 0x34, 0x12, 0xFE, 0x03,  # imgol image 0x1234, offsets -2,+3
            9, 0x78, 0x56, 0x01, 0xFF,  # imgul image 0x5678, offsets +1,-1
            36, 7,                       # metadata only
            5, 0, 22,
        ))
        result = self.compile(code)
        self.assertEqual(result["omitted_layers"], [
            {"opcode": 8, "image_id": 0x1234, "x_offset": -2,
             "y_offset": 3},
            {"opcode": 9, "image_id": 0x5678, "x_offset": 1,
             "y_offset": -1},
        ])
        self.assertEqual(result["signals"], [7])

    def test_rejects_unsupported_unknown_branch_random_and_call(self) -> None:
        for opcode in (1, 6, 10, 13, 39, 45, 64, 255):
            with self.subTest(opcode=opcode), self.assertRaises(ValueError):
                self.compile(bytes((opcode,)))

    def test_rejects_out_of_range_start_frames_and_goto(self) -> None:
        with self.assertRaises(ValueError):
            compile_primary_frame_stream(b"", 0, 1)
        with self.assertRaises(ValueError):
            self.compile(play(1), frame_count=1)
        with self.assertRaises(ValueError):
            self.compile(goto(4) + bytes((22,)))

    def test_rejects_truncated_operands(self) -> None:
        cases = (b"\x00", b"\x00\x01", b"\x05", b"\x07",
                 b"\x07\x01", b"\x08\x01\x00\x00",
                 b"\x36")
        for code in cases:
            with self.subTest(code=code), self.assertRaises(ValueError):
                self.compile(code)

    def test_rejects_fallthrough_and_unrepresentable_frame_bank(self) -> None:
        with self.assertRaisesRegex(ValueError, "falls off"):
            self.compile(wait(0))
        with self.assertRaises(ValueError):
            self.compile(bytes((22,)), frame_count=256)

    def test_rejects_instantaneous_loop(self) -> None:
        with self.assertRaisesRegex(ValueError, "no wait"):
            self.compile(goto(0))

    def test_rejects_step_and_instruction_limits(self) -> None:
        code = b"".join(wait(0) for _ in range(65)) + bytes((22,))
        with self.assertRaisesRegex(ValueError, "64 steps"):
            self.compile(code)
        code = bytes((36, 0)) * 257 + bytes((22,))
        with self.assertRaisesRegex(ValueError, "256 instructions"):
            self.compile(code)

    def test_source_compiler_tracks_spawned_image_with_offsets_and_timing(self) -> None:
        # Root waits one update, then places image 20 at (+1,-2), waits, ends.
        # Child image's Init starts at byte 10 and draws frame 1 for two ticks.
        code = bytes((5, 0, 8, 20, 0, 1, 254, 5, 0, 22,
                      0, 1, 0, 5, 1, 7, 10, 0))
        layers = compile_source_layers(
            code, {10: 0}, 10, decode_instruction, [0],
            {(20, "Init"): 10}, {10: 2, 20: 2})
        self.assertEqual(len(layers), 2)
        self.assertEqual(layers[0]["image_id"], 10)
        self.assertEqual(layers[1]["image_id"], 20)
        self.assertEqual((layers[1]["x_offset"], layers[1]["y_offset"]), (1, -2))
        self.assertEqual(layers[1]["start_source_updates"], 1)
        self.assertEqual(layers[1]["program"]["steps"], [
            {"frame": 1, "duration_source_updates": 2}])

    def test_follow_main_and_gas_overlays_are_explicit_layer_metadata(self) -> None:
        # followmaingraphic (29), creategasoverlays (56) with signed selector,
        # then end. Gas requests remain dynamic and are not folded into GRPs.
        code = bytes((5, 0, 29, 5, 1, 56, 0xFE, 22))
        layers = compile_source_layers(
            code, {10: 0}, 10, decode_instruction, [0], {}, {10: 1})
        self.assertTrue(layers[0]["follows_main_graphic"])
        self.assertEqual(layers[0]["gas_overlay_requests"], [
            {"selector": -2, "start_source_updates": 3}])
        self.assertEqual(layers[0]["program"]["steps"], [
            {"frame": 0, "duration_source_updates": 1},
            {"frame": 0, "duration_source_updates": 2}])
        # Opcode 41 is move speed and consumes one byte without changing the
        # construction animation's visual clock.
        moved = compile_source_layers(bytes((41, 99, 22)), {10: 0}, 10,
                                     decode_instruction, [0], {}, {10: 1})
        self.assertEqual(moved[0]["gas_overlay_requests"], [])
        with self.assertRaisesRegex(ValueError, "setflipstate"):
            compile_source_layers(bytes((23, 1, 22)), {10: 0}, 10,
                                 decode_instruction, [0], {}, {10: 1})


if __name__ == "__main__":
    unittest.main()
