"""Asset-free tests for bounded Z8 AI program decoding."""
from __future__ import annotations

import importlib.util
from pathlib import Path
import struct
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    'sc64_ai_programs_test', ROOT / 'tools/sc64_ai_programs.py')
assert SPEC and SPEC.loader
programs = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = programs
SPEC.loader.exec_module(programs)

CATALOG_SPEC = importlib.util.spec_from_file_location(
    'sc64_ai_catalog_for_program_tests', ROOT / 'tools/sc64_ai_catalog.py')
assert CATALOG_SPEC and CATALOG_SPEC.loader
catalog_tool = importlib.util.module_from_spec(CATALOG_SPEC)
sys.modules[CATALOG_SPEC.name] = catalog_tool
CATALOG_SPEC.loader.exec_module(catalog_tool)


def catalog_for(code: bytes, starts: tuple[int, int, int]) -> tuple[bytes, dict]:
    table = 4 + len(code)
    rows = bytearray()
    for script_id, start in zip(programs.TARGET_IDS, starts):
        rows.extend(struct.pack('<4sIII', script_id.encode('ascii'), start, 1, 3))
    payload = struct.pack('<I', table) + code + bytes(rows) + b'\0' * 4
    return payload, catalog_tool.parse_catalog(payload)


class Sc64AiProgramTests(unittest.TestCase):
    def test_decodes_known_command_operands_and_offsets(self):
        # wait(1), build(1, type 154, priority 150), stop
        data = b'\0' * 4 + bytes((2, 1, 0, 6, 1, 154, 0, 150, 36))
        decoded = programs.decode_segment(data, 4, len(data))
        self.assertEqual([(row['name'], row['operands']) for row in decoded], [
            ('wait', [1]), ('build', [1, 154, 150]), ('stop', []),
        ])
        self.assertEqual([row['offset'] for row in decoded], [4, 7, 12])

    def test_branch_can_target_shared_code_at_next_script_start(self):
        # Zer8's segment ends after goto; the explicit goto enters Ze8T's code.
        code = bytes((0, 7, 0, 36, 38, 36))
        payload, catalog = catalog_for(code, (4, 7, 8))
        result = programs.analyze_catalog(payload, catalog)
        zerg = result['programs'][0]
        self.assertEqual(zerg['goto_targets'], [7])
        self.assertEqual(zerg['instructions'][0]['branch_target'], 7)
        self.assertEqual(result['programs'][2]['op_counts'], {'move_dt': 1, 'stop': 1})

    def test_unknown_opcode_and_truncated_operands_fail_closed(self):
        with self.assertRaisesRegex(ValueError, 'unsupported AI opcode'):
            programs.decode_segment(b'\0' * 4 + bytes((0x71,)), 4, 5)
        with self.assertRaisesRegex(ValueError, 'truncated build'):
            programs.decode_segment(b'\0' * 4 + bytes((6, 1, 154)), 4, 7)

    def test_jump_into_operand_is_rejected_as_nonboundary(self):
        code = bytes((0, 5, 0, 2, 1, 0, 36))
        payload, catalog = catalog_for(code, (4, 7, 8))
        with self.assertRaisesRegex(ValueError, 'not an instruction boundary'):
            programs.analyze_catalog(payload, catalog)

    def test_goto_outside_leaf_code_is_rejected(self):
        code = bytes((0, 0xff, 0x7f, 36, 38, 36))
        payload, catalog = catalog_for(code, (4, 7, 8))
        with self.assertRaisesRegex(ValueError, 'outside code/table bounds'):
            programs.analyze_catalog(payload, catalog)


if __name__ == '__main__':
    unittest.main()
