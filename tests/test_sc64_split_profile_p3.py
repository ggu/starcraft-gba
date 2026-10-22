import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from sc64_split_profile import decode_p3_state


class P3ObserverTests(unittest.TestCase):
    def test_decodes_nonstandard_layout_and_pending_action_timer(self):
        offsets = {"fired": 6, "outcome": 1, "sequence": 3, "transmission": 9, "reserved": 0}
        raw = bytearray(12)
        raw[6:8] = (0x1234).to_bytes(2, "little")
        raw[1], raw[3], raw[9] = 0, 2, 1
        self.assertEqual(decode_p3_state(raw, offsets, 87), {
            "fired": 0x1234, "outcome": 0, "sequence": 2,
            "transmission": 1, "reserved": 0, "action_ticks": 87})

    def test_rejects_missing_or_out_of_bounds_fields(self):
        offsets = {"fired": 0, "outcome": 2, "sequence": 3, "transmission": 4, "reserved": 5}
        with self.assertRaisesRegex(RuntimeError, "incomplete"):
            decode_p3_state(bytes(6), {k: v for k, v in offsets.items() if k != "fired"}, 0)
        for field in offsets:
            for invalid in (-1, 6):
                with self.subTest(field=field, offset=invalid):
                    with self.assertRaisesRegex(RuntimeError, "outside"):
                        decode_p3_state(bytes(6), {**offsets, field: invalid}, 0)
        with self.assertRaisesRegex(RuntimeError, "outside"):
            decode_p3_state(bytes(1), offsets, 0)


if __name__ == "__main__":
    unittest.main()
