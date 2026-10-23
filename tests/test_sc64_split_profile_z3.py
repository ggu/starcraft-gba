import sys
import unittest
from pathlib import Path

TOOLS = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS))

import sc64_split_profile as profile


class Z3ObservationTests(unittest.TestCase):
    def test_decodes_model_members_and_action_timer(self):
        # Nonsequential offsets make the test verify DWARF lookup rather than
        # accidentally relying on the C structure's current byte layout.
        raw = bytearray(8)
        raw[4:6] = (0x0105).to_bytes(2, "little")
        raw[0] = 1
        raw[2] = 2
        raw[3] = 7
        raw[6] = 3
        result = profile.decode_z3_state(
            bytes(raw), {"fired": 4, "switch7": 0, "outcome": 2,
                         "sequence": 3, "transmission": 6}, 9)
        self.assertEqual(result, {"fired": 0x0105, "switch7": 1, "outcome": 2,
                                  "sequence": 7, "transmission": 3,
                                  "action_ticks": 9})

    def test_rejects_incomplete_or_out_of_bounds_dwarf_layout(self):
        with self.assertRaisesRegex(RuntimeError, "incomplete"):
            profile.decode_z3_state(b"\0" * 8, {"fired": 0}, 0)
        offsets = {"fired": 7, "switch7": 0, "outcome": 1,
                   "sequence": 2, "transmission": 3}
        with self.assertRaisesRegex(RuntimeError, "outside"):
            profile.decode_z3_state(b"\0" * 8, offsets, 0)


if __name__ == "__main__":
    unittest.main()
