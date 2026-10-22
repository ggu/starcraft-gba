import sys
import unittest
from pathlib import Path

TOOLS = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS))

import sc64_z3_controller_replay as replay


def valid_trace():
    return {
        "schema": 1,
        "mission": 14,
        "policy": "fixture data only",
        "events": [
            {"kind": "input", "host_tick": 1, "held": 1, "pressed": 1},
            {"kind": "input", "host_tick": 2, "held": 0, "pressed": 0},
            {"kind": "input", "host_tick": 3, "held": 16, "pressed": 16},
            {"kind": "input", "host_tick": 4, "held": 0, "pressed": 0},
            {"kind": "input", "host_tick": 5, "held": 1, "pressed": 1},
            {"kind": "input", "host_tick": 6, "held": 0, "pressed": 0},
            {"kind": "input", "host_tick": 7, "held": 1, "pressed": 1},
            {"kind": "start", "host_tick": 7, "frame": 0, "screen": 1},
            {"kind": "input", "host_tick": 8, "held": 256, "pressed": 256},
            {"kind": "input", "host_tick": 9, "held": 0, "pressed": 0},
            {"kind": "won", "host_tick": 71186, "frame": 70833, "screen": 3,
             "outcome": 1, "win_presence_condition": 1, "foes_any": 4,
             "side_units": 79, "side_buildings": 8, "enemy_units": 3,
             "enemy_buildings": 1, "ore": 157, "gas": 1359,
             "enemy_ore": 10284, "enemy_gas": 8000},
        ],
    }


class TraceValidationTests(unittest.TestCase):
    def test_host_ticks_map_to_zero_based_scheduler_ticks(self):
        plan = replay.validate_trace(valid_trace(), capacity=16)
        self.assertEqual(plan["stop_tick"], 71186)
        self.assertEqual(plan["inputs"][0], (0, 1))
        self.assertEqual(plan["inputs"][6], (6, 1))
        self.assertEqual(replay.scheduler_events(plan), plan["inputs"])

    def test_rejects_wrong_schema_or_mission(self):
        trace = valid_trace()
        trace["schema"] = 2
        with self.assertRaisesRegex(ValueError, "schema"):
            replay.validate_trace(trace, 16)
        trace = valid_trace()
        trace["mission"] = 13
        with self.assertRaisesRegex(ValueError, "global ID 14"):
            replay.validate_trace(trace, 16)

    def test_rejects_event_capacity_overflow(self):
        with self.assertRaisesRegex(ValueError, "capacity"):
            replay.validate_trace(valid_trace(), capacity=8)

    def test_rejects_nonincreasing_ticks_and_nontransitions(self):
        trace = valid_trace()
        trace["events"][1]["host_tick"] = 1
        with self.assertRaisesRegex(ValueError, "strictly increasing"):
            replay.validate_trace(trace, 16)
        trace = valid_trace()
        trace["events"][1]["held"] = 1
        trace["events"][1]["pressed"] = 0
        with self.assertRaisesRegex(ValueError, "not a held-state transition"):
            replay.validate_trace(trace, 16)

    def test_rejects_pressed_mismatch_and_out_of_range_mask(self):
        trace = valid_trace()
        trace["events"][0]["pressed"] = 0
        with self.assertRaisesRegex(ValueError, "pressed does not match"):
            replay.validate_trace(trace, 16)
        trace = valid_trace()
        trace["events"][0]["held"] = 0x400
        trace["events"][0]["pressed"] = 0x400
        with self.assertRaisesRegex(ValueError, "outside 0x3ff"):
            replay.validate_trace(trace, 16)

    def test_rejects_missing_start_or_final_predicate(self):
        trace = valid_trace()
        trace["events"] = [event for event in trace["events"] if event["kind"] != "start"]
        with self.assertRaisesRegex(ValueError, "start event"):
            replay.validate_trace(trace, 16)
        trace = valid_trace()
        trace["events"][-1]["win_presence_condition"] = 0
        with self.assertRaisesRegex(ValueError, "victory predicate"):
            replay.validate_trace(trace, 16)

    def test_rejects_unknown_event_kinds(self):
        trace = valid_trace()
        trace["events"].insert(-1, {"kind": "lost", "host_tick": 10})
        with self.assertRaisesRegex(ValueError, "unsupported kind"):
            replay.validate_trace(trace, 16)

    def test_rejects_final_tick_beyond_u32_or_changed_proof_values(self):
        trace = valid_trace()
        trace["events"][-1]["host_tick"] = 0x1_0000_0000
        with self.assertRaisesRegex(ValueError, "32-bit"):
            replay.validate_trace(trace, 16)
        trace = valid_trace()
        trace["events"][-1]["frame"] = 70832
        with self.assertRaisesRegex(ValueError, "verified Z3 controller trace"):
            replay.validate_trace(trace, 16)

    def test_capacity_is_derived_from_elf_symbol_size(self):
        self.assertEqual(replay.schedule_capacity({"demo_schedule_events": 2048 * 8}), 2048)
        with self.assertRaisesRegex(RuntimeError, "symbol size"):
            replay.schedule_capacity({})
        with self.assertRaisesRegex(RuntimeError, "symbol size"):
            replay.schedule_capacity({"demo_schedule_events": 17})

    def test_prefix_length_and_save_options_are_bounded(self):
        plan = replay.validate_trace(valid_trace(), 16)
        self.assertEqual(replay.run_length(plan, None), (71186, False))
        self.assertEqual(replay.run_length(plan, 2500), (2500, True))
        with self.assertRaisesRegex(ValueError, "positive 32-bit"):
            replay.run_length(plan, 0)
        with self.assertRaisesRegex(ValueError, "positive 32-bit"):
            replay.run_length(plan, 0x1_0000_0000)
        with self.assertRaisesRegex(ValueError, "cannot exceed"):
            replay.run_length(plan, 71187)
        with self.assertRaisesRegex(ValueError, "requires a --ticks"):
            replay.run_length(plan, None, save_at_end=True)
        with self.assertRaisesRegex(ValueError, "before the trace stop"):
            replay.run_length(plan, 71186, save_at_end=True)
        self.assertEqual(replay.run_length(plan, 1000, save_at_end=True), (1000, True))

    def test_route_interval_delta_handles_u32_wrap(self):
        self.assertEqual(replay.route_counter_deltas({"sc64_route_hits": 5,
                                                       "sc64_route_builds": 10},
                                                      {"sc64_route_hits": 0xFFFFFFFE,
                                                       "sc64_route_builds": 8}),
                         {"sc64_route_hits": 7, "sc64_route_builds": 2})


if __name__ == "__main__":
    unittest.main()
