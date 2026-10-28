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
            {"kind": "input", "host_tick": 7, "held": 8, "pressed": 8},
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
        self.assertEqual(plan["inputs"][6], (6, replay.KEY_START))
        self.assertEqual(replay.scheduler_events(plan), plan["inputs"])

    def test_normal_menu_deployment_requires_start_at_the_existing_tick(self):
        trace = valid_trace()
        trace["events"][6]["held"] = 1
        trace["events"][6]["pressed"] = 1
        with self.assertRaisesRegex(ValueError, "must use Start"):
            replay.validate_trace(trace, capacity=16)

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

    def test_scheduler_mode_requires_both_window_symbols(self):
        self.assertEqual(replay.schedule_mode({}, {"demo_schedule_events": 2048 * 8}),
                         ("full", 2048))
        windowed_symbols = {"demo_schedule_active": 0x1000, "demo_schedule_index": 0x1004}
        sizes = {"demo_schedule_events": 256 * 8,
                 "demo_schedule_active": 4, "demo_schedule_index": 4}
        self.assertEqual(replay.schedule_mode(windowed_symbols, sizes), ("windowed", 256))
        with self.assertRaisesRegex(RuntimeError, "incomplete windowed"):
            replay.schedule_mode({"demo_schedule_active": 0x1000}, sizes)
        with self.assertRaisesRegex(RuntimeError, "unexpected size"):
            replay.schedule_mode(windowed_symbols, {**sizes, "demo_schedule_index": 2})

    def test_windowed_capacity_is_per_checkpoint_window_not_total_trace(self):
        trace = valid_trace()
        rows = trace["events"]
        # Add 300 real held-state transitions distributed across two windows.
        held = 0
        added = []
        for i in range(300):
            next_held = 1 if held == 0 else 0
            tick = 10 + i * 7
            added.append({"kind": "input", "host_tick": tick, "held": next_held,
                          "pressed": next_held & ~held})
            held = next_held
        rows[-1:-1] = added
        with self.assertRaisesRegex(ValueError, "capacity"):
            replay.validate_trace(trace, capacity=256)
        plan = replay.validate_trace(trace, capacity=256, windowed=True)
        self.assertEqual(plan["input_count"], 309)
        self.assertLessEqual(plan["peak_page_transitions"], 256)
        self.assertGreater(plan["peak_page_transitions"], 0)

    def test_windowed_rejects_overcapacity_single_window(self):
        trace = valid_trace()
        rows = trace["events"]
        held = 0
        added = []
        for i in range(260):
            next_held = 1 if held == 0 else 0
            tick = 10 + i
            added.append({"kind": "input", "host_tick": tick, "held": next_held,
                          "pressed": next_held & ~held})
            held = next_held
        rows[-1:-1] = added
        with self.assertRaisesRegex(ValueError, "event page exceeds ELF capacity"):
            replay.validate_trace(trace, capacity=256, windowed=True)

    def test_window_pages_preserve_absolute_ticks_and_partial_final_window(self):
        plan = {"inputs": [(0, 1), (999, 0), (1000, 1), (1499, 0), (1500, 1)]}
        pages = replay.make_schedule_pages(plan, requested_ticks=1500, capacity=4)
        self.assertEqual([(p["first"], p["end"]) for p in pages], [(0, 1000), (1000, 1500)])
        self.assertEqual(pages[0]["events"], [(0, 1), (999, 0)])
        self.assertEqual(pages[1]["events"], [(1000, 1), (1499, 0)])

    def test_window_pages_reject_malformed_or_overcapacity_plan_before_run(self):
        with self.assertRaisesRegex(ValueError, "unordered"):
            replay.make_schedule_pages({"inputs": [(1, 1), (1, 0)]}, 10, 4)
        with self.assertRaisesRegex(ValueError, "held bits"):
            replay.make_schedule_pages({"inputs": [(0, 0x400)]}, 10, 4)
        with self.assertRaisesRegex(ValueError, "event page exceeds"):
            replay.make_schedule_pages({"inputs": [(0, 1), (1, 0), (2, 1)]}, 10, 2)

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
