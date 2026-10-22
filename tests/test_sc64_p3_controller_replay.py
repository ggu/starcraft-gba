import unittest

from tools import sc64_p3_controller_replay as replay
from tools import sc64_z3_controller_replay as scheduler


def valid_trace():
    final = dict(replay.EXPECTED_FINAL, elapsed=replay.EXPECTED_FINAL["frame"],
                 side_army=13, geyser_count=3)
    return {"schema": 1, "mission": 22, "policy": "synthetic-controller-test", "events": [
        {"kind": "input", "host_tick": 1, "held": 1, "pressed": 1},
        {"kind": "input", "host_tick": 2, "held": 0, "pressed": 0},
        {"kind": "input", "host_tick": 3, "held": 4, "pressed": 4},
        {"kind": "start", "host_tick": 7, "frame": 0, "screen": 1,
         "p3_outcome": 0, "won": 0, "lost": 0},
        {"kind": "periodic", "host_tick": 1000, "frame": 992},
        {"kind": "input", "host_tick": 1001, "held": 0, "pressed": 0},
        {"kind": "won", **final},
    ]}


class P3ControllerReplayValidation(unittest.TestCase):
    def test_valid_trace_full_queue(self):
        plan = replay.validate_trace(valid_trace(), 4)
        self.assertEqual(plan["input_count"], 4)
        self.assertEqual(plan["stop_tick"], 152107)
        self.assertEqual(plan["scheduler_mode"], "full")
        self.assertEqual(plan["inputs"][0], (0, 1))

    def test_windowed_trace_capacity_and_page_boundaries(self):
        trace = valid_trace()
        plan = replay.validate_trace(trace, 3, windowed=True)
        pages = scheduler.make_schedule_pages(plan, 1500, 3)
        self.assertEqual([len(page["events"]) for page in pages[:2]], [3, 1])
        self.assertEqual(pages[0]["first"], 0)
        self.assertEqual(pages[1]["first"], 1000)

    def test_rejects_legacy_queue_overflow_and_window_overflow(self):
        trace = valid_trace()
        with self.assertRaisesRegex(ValueError, "queue exceeds"):
            replay.validate_trace(trace, 3)
        with self.assertRaisesRegex(ValueError, "page exceeds"):
            replay.validate_trace(trace, 2, windowed=True)

    def test_rejects_bad_inputs_and_final_contract(self):
        mutations = [
            lambda t: t.update(mission=14),
            lambda t: t["events"][1].update(pressed=1),
            lambda t: t["events"][2].update(held=1),
            lambda t: t["events"][-1].update(force3_buildings=1),
            lambda t: t["events"][-1].update(p3_sequence=4),
            lambda t: t["events"][-1].update(gas=1047),
        ]
        for mutate in mutations:
            with self.subTest(mutate=mutate):
                trace = valid_trace()
                mutate(trace)
                with self.assertRaises(ValueError):
                    replay.validate_trace(trace, 20, windowed=True)

    def test_rejects_missing_or_misordered_terminal_events(self):
        trace = valid_trace()
        trace["events"].pop()
        with self.assertRaisesRegex(ValueError, "end with its final won"):
            replay.validate_trace(trace, 20, windowed=True)
        trace = valid_trace()
        trace["events"].append(trace["events"].pop(3))
        with self.assertRaisesRegex(ValueError, "end with its final won"):
            replay.validate_trace(trace, 20, windowed=True)

    def test_final_verifier_requires_completion_bit_and_model(self):
        state = {
            "screen": 3, "frame": 151088, "won": 1, "lost": 0,
            "side_units": 14, "side_buildings": 11,
            "force2_units": 7, "force2_buildings": 0,
            "force3_units": 0, "force3_buildings": 0,
            "minerals": 44, "gas": 1048, "race_completed": 1 << 22,
            "race_mode": 0, "race_selected_mission": 22,
            "supply": 54, "supply_cap": 98, "gateways": 1,
            # SIDE wallet arrays are stale mirrors; globals are authoritative.
            "race_minerals": [300, 0, 0, 0, 0], "race_gas": [100, 0, 0, 0, 0],
            "race_campaign": {"mission": 22, "elapsed_frames": 151088, "phase": 3,
                              "objective_flags": 1},
            "race_p3": {"outcome": 1, "sequence": 5, "fired": 31,
                        "transmission": 0, "reserved": 0, "action_ticks": 0},
        }
        replay.verify_final(valid_trace()["events"][-1], state)
        stale_wallet_arrays = dict(state)
        stale_wallet_arrays["race_minerals"] = [999, 0, 0, 0, 0]
        stale_wallet_arrays["race_gas"] = [888, 0, 0, 0, 0]
        replay.verify_final(valid_trace()["events"][-1], stale_wallet_arrays)
        state["race_completed"] = 0
        with self.assertRaisesRegex(RuntimeError, "completion bit"):
            replay.verify_final(valid_trace()["events"][-1], state)

    def test_final_verifier_rejects_wrong_mode_mission_and_supply(self):
        state = {
            "screen": 3, "frame": 151088, "won": 1, "lost": 0,
            "side_units": 14, "side_buildings": 11,
            "force2_units": 7, "force2_buildings": 0,
            "force3_units": 0, "force3_buildings": 0,
            "minerals": 44, "gas": 1048, "race_completed": 1 << 22,
            "race_mode": 0, "race_selected_mission": 22,
            "supply": 54, "supply_cap": 98, "gateways": 1,
            "race_minerals": [300, 0, 0, 0, 0], "race_gas": [100, 0, 0, 0, 0],
            "race_campaign": {"mission": 22, "elapsed_frames": 151088, "phase": 3,
                              "objective_flags": 1},
            "race_p3": {"outcome": 1, "sequence": 5, "fired": 31,
                        "transmission": 0, "reserved": 0, "action_ticks": 0},
        }
        final = valid_trace()["events"][-1]
        for key, value, message in (("race_mode", 1, "race_mode"),
                                    ("race_selected_mission", 21, "race_selected_mission"),
                                    ("supply", 55, "supply"),
                                    ("minerals", 43, "minerals"),
                                    ("gas", 1047, "gas")):
            changed = dict(state)
            changed[key] = value
            with self.subTest(key=key), self.assertRaisesRegex(RuntimeError, message):
                replay.verify_final(final, changed)


if __name__ == "__main__":
    unittest.main()
