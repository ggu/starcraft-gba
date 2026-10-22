import struct
import tempfile
import unittest
from pathlib import Path

from tools.sc64_import import emit_header, nonzero_fields, unit_rows


class Sc64ImportTests(unittest.TestCase):
    def test_unit_parser_preserves_raw_ids_and_owner(self):
        record = bytearray(36)
        struct.pack_into("<HHHHHHH", record, 4,
                         128, 176, 0, 1, 0, 0, 0)
        record[16:20] = bytes((1, 100, 0, 0))
        struct.pack_into("<I", record, 20, 0)
        struct.pack_into("<H", record, 26, 0)
        row = unit_rows(bytes(record))[0]
        self.assertEqual((row["x_source_px"], row["y_source_px"]), (128, 176))
        self.assertEqual((row["source_type"], row["owner_raw"]), (0, 1))
        self.assertEqual(row["hp_percent"], 100)

    def test_trigger_condition_and_action_decode_full_ids(self):
        condition = bytearray(20)
        struct.pack_into("<IIIHBBBBH", condition, 0,
                         0, 1, 10, 0, 0, 2, 0, 0, 0)
        action = bytearray(32)
        struct.pack_into("<IIIIIIHBBB", action, 0,
                         0, 0, 0, 0, 0, 0, 0, 1, 0, 0)
        conditions = nonzero_fields(bytes(condition), 0, 20, "condition")
        actions = nonzero_fields(bytes(action), 0, 32, "action")
        self.assertEqual(conditions[0]["name"], "Command")
        self.assertEqual((conditions[0]["player_group"], conditions[0]["amount"]), (1, 10))
        self.assertEqual(actions[0]["name"], "Victory")

    def test_header_preserves_full_u16_unit_flags(self):
        unit = {
            "source_type": 0, "x_source_px": 32, "y_source_px": 48,
            "owner_raw": 1, "hp_percent": 100, "flags": 0x0100,
            "resources": 0,
        }
        with tempfile.TemporaryDirectory() as temp:
            output = Path(temp) / "mission.h"
            emit_header(output, {"width": 1, "height": 1}, [unit], [0], [])
            header = output.read_text()
        self.assertIn("16u, 24u, 0u, 1u, 100u, 256u, 0u", header)


if __name__ == "__main__":
    unittest.main()
