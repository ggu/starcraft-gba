"""Synthetic tests for the offline P7 controller JSONL trace validator."""
from __future__ import annotations

import importlib.util
import json
from pathlib import Path
import tempfile
import unittest


TOOL = Path(__file__).resolve().parents[1] / "tools" / "sc64_p7_campaign_trace.py"
SPEC = importlib.util.spec_from_file_location("sc64_p7_campaign_trace", TOOL)
assert SPEC and SPEC.loader
trace_tool = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(trace_tool)


def valid_rows():
    return [
        {"kind": "metadata", "schema": 1, "mission": 26,
         "input_policy": "ordinary game_update held/pressed only"},
        {"kind": "input", "call_index": 0, "frame_before": 0,
         "frame_after": 1, "held": 1, "pressed": 1},
        {"kind": "input", "call_index": 4, "frame_before": 3,
         "frame_after": 4, "held": 0, "pressed": 0},
        {"kind": "final", "total_calls": 5, "frame": 4,
         "screen": 3, "won": 1, "lost": 0, "heroes_alive": 3},
        {"kind": "strict-verdict", "ok": 1, "screen_result": 1,
         "won": 1, "lost": 0, "outcome": 1, "fault": 0,
         "switch_bits": 1, "defeat_started": 0, "victory_started": 1,
         "victory_cursor": 0, "action_timers_clear": 1,
         "force2_nexus_condition": 0, "force3_nexus_condition": 1,
         "heroes_alive": 3},
    ]


class P7CampaignTraceTests(unittest.TestCase):
    def test_valid_full_win_contract(self):
        result = trace_tool.validate_trace(valid_rows(), require_win=True)
        self.assertEqual(result["input_transitions"], 2)
        self.assertEqual(result["last_input_call_index"], 4)
        self.assertEqual(result["total_calls"], 5)
        self.assertEqual(result["edge_errors"], 0)

    def test_duplicate_json_keys_rejected(self):
        with self.assertRaisesRegex(trace_tool.TraceValidationError, "duplicate JSON key"):
            trace_tool.parse_jsonl('{"kind":"metadata","kind":"metadata"}\n')

    def test_metadata_cardinality_and_schema(self):
        rows = valid_rows()
        rows.insert(1, dict(rows[0]))
        with self.assertRaisesRegex(trace_tool.TraceValidationError, "exactly one metadata"):
            trace_tool.validate_trace(rows)
        rows = valid_rows()
        rows[0]["schema"] = True
        with self.assertRaisesRegex(trace_tool.TraceValidationError, "schema must be integer 1"):
            trace_tool.validate_trace(rows)

    def test_first_and_intervening_call_frame_feasibility(self):
        rows = valid_rows()
        rows[1]["call_index"] = 2
        rows[1]["frame_before"] = 1
        rows[1]["frame_after"] = 2
        rows[2]["call_index"] = 5
        rows[2]["frame_before"] = 3
        rows[2]["frame_after"] = 4
        rows[3]["total_calls"] = 6
        self.assertEqual(trace_tool.validate_trace(rows)["total_calls"], 6)
        rows = valid_rows()
        rows[1]["call_index"] = 0
        rows[1]["frame_before"] = 1
        rows[1]["frame_after"] = 2
        with self.assertRaisesRegex(trace_tool.TraceValidationError, "first input frame_before"):
            trace_tool.validate_trace(rows)
        rows = valid_rows()
        rows[2]["frame_before"] = 5
        rows[2]["frame_after"] = 6
        rows[3]["frame"] = 6
        with self.assertRaisesRegex(trace_tool.TraceValidationError, "advance between input calls"):
            trace_tool.validate_trace(rows)

    def test_bool_is_not_integer(self):
        rows = valid_rows()
        rows[1]["held"] = True
        with self.assertRaisesRegex(trace_tool.TraceValidationError, "held must be"):
            trace_tool.validate_trace(rows)

    def test_negative_and_nonincreasing_call_indices_rejected(self):
        rows = valid_rows()
        rows[1]["call_index"] = -1
        with self.assertRaisesRegex(trace_tool.TraceValidationError, "call_index"):
            trace_tool.validate_trace(rows)
        rows = valid_rows()
        rows[2]["call_index"] = 0
        with self.assertRaisesRegex(trace_tool.TraceValidationError, "strictly increasing"):
            trace_tool.validate_trace(rows)

    def test_changed_held_masks_and_press_edges_rejected(self):
        rows = valid_rows()
        rows[2]["held"] = 1
        with self.assertRaisesRegex(trace_tool.TraceValidationError, "changed mask"):
            trace_tool.validate_trace(rows)
        rows = valid_rows()
        rows[2]["pressed"] = 1
        with self.assertRaisesRegex(trace_tool.TraceValidationError, "pressed does not match"):
            trace_tool.validate_trace(rows)
        rows = valid_rows()
        rows[1]["held"] = 0x400
        rows[1]["pressed"] = 0x400
        with self.assertRaisesRegex(trace_tool.TraceValidationError, "changed mask"):
            trace_tool.validate_trace(rows)

    def test_frame_delta_and_order_rejected(self):
        rows = valid_rows()
        rows[1]["frame_after"] = 2
        with self.assertRaisesRegex(trace_tool.TraceValidationError, "delta"):
            trace_tool.validate_trace(rows)
        rows = valid_rows()
        rows[2]["frame_before"] = 0
        rows[2]["frame_after"] = 1
        with self.assertRaisesRegex(trace_tool.TraceValidationError, "move backwards"):
            trace_tool.validate_trace(rows)

    def test_total_calls_and_final_frame_are_coherent(self):
        for total_calls in (0, True, 4):
            rows = valid_rows()
            rows[-2]["total_calls"] = total_calls
            with self.subTest(total_calls=total_calls), self.assertRaises(trace_tool.TraceValidationError):
                trace_tool.validate_trace(rows)
        rows = valid_rows()
        rows[-2]["frame"] = 3
        with self.assertRaisesRegex(trace_tool.TraceValidationError, "final.frame"):
            trace_tool.validate_trace(rows)
        rows = valid_rows()
        rows[-2]["frame"] = 6
        with self.assertRaisesRegex(trace_tool.TraceValidationError, "final.frame"):
            trace_tool.validate_trace(rows)
        rows = valid_rows()
        rows[-2]["total_calls"] = 5
        rows[-2]["frame"] = 5
        with self.assertRaisesRegex(trace_tool.TraceValidationError, "advances too far after"):
            trace_tool.validate_trace(rows)

    def test_final_and_verdict_cardinality_and_order(self):
        rows = valid_rows()
        rows.append(dict(rows[-1]))
        with self.assertRaisesRegex(trace_tool.TraceValidationError, "exactly one"):
            trace_tool.validate_trace(rows)
        rows = valid_rows()
        rows[-1], rows[-2] = rows[-2], rows[-1]
        with self.assertRaisesRegex(trace_tool.TraceValidationError, "strict-verdict must be"):
            trace_tool.validate_trace(rows)
        rows = valid_rows()
        rows.insert(-1, dict(rows[-1]))
        with self.assertRaisesRegex(trace_tool.TraceValidationError, "exactly one"):
            trace_tool.validate_trace(rows)

    def test_no_input_after_final(self):
        rows = valid_rows()
        rows.insert(-1, {"kind": "input", "call_index": 5,
                         "frame_before": 4, "frame_after": 4,
                         "held": 2, "pressed": 2})
        rows[3]["total_calls"] = 6
        with self.assertRaisesRegex(trace_tool.TraceValidationError, "input rows cannot follow"):
            trace_tool.validate_trace(rows)

    def test_win_contract_fields_are_strictly_checked(self):
        rows = valid_rows()
        rows[4]["force3_nexus_condition"] = 0
        with self.assertRaisesRegex(trace_tool.TraceValidationError, "Nexus victory"):
            trace_tool.validate_trace(rows, require_win=True)
        rows = valid_rows()
        rows[4]["heroes_alive"] = True
        with self.assertRaisesRegex(trace_tool.TraceValidationError, "heroes_alive"):
            trace_tool.validate_trace(rows, require_win=True)
        rows = valid_rows()
        rows[2]["held"] = 2
        rows[2]["pressed"] = 2
        with self.assertRaisesRegex(trace_tool.TraceValidationError, "end with neutral"):
            trace_tool.validate_trace(rows, require_win=True)

    def test_file_digest_and_utf8_validation(self):
        rows = valid_rows()
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / "trace.jsonl"
            path.write_text("\n".join(json.dumps(row) for row in rows) + "\n")
            result = trace_tool.validate_file(path, require_win=True)
            self.assertEqual(len(result["sha256"]), 64)
            path.write_bytes(b"\xff")
            with self.assertRaisesRegex(trace_tool.TraceValidationError, "UTF-8"):
                trace_tool.validate_file(path)


if __name__ == "__main__":
    unittest.main()
