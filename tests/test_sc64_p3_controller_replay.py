import unittest

from tools import sc64_p3_controller_replay as replay
from tools import sc64_z3_controller_replay as scheduler


def valid_trace():
    final = dict(replay.EXPECTED_FINAL, elapsed=replay.EXPECTED_FINAL["frame"],
                 side_army=13, geyser_count=3)
    return {"schema": 1, "mission": 22, "policy": replay.GARRISON_POLICY, "events": [
        {"kind": "input", "host_tick": 1, "held": 1, "pressed": 1},
        {"kind": "input", "host_tick": 2, "held": 0, "pressed": 0},
        {"kind": "input", "host_tick": 3, "held": 4, "pressed": 4},
        {"kind": "start", "host_tick": 7, "frame": 0, "screen": 1,
         "p3_outcome": 0, "won": 0, "lost": 0},
        {"kind": "periodic", "host_tick": 1000, "frame": 992},
        {"kind": "input", "host_tick": 1001, "held": 0, "pressed": 0},
        {"kind": "won", **final},
    ]}


def valid_expansion_trace():
    final = dict(replay.EXPANSION_EXPECTED_FINAL, elapsed=replay.EXPANSION_EXPECTED_FINAL["frame"])
    start = dict(replay.EXPANSION_EXPECTED_START, kind="start", host_tick=7, frame=0,
                 screen=1, won=0, lost=0, p3_outcome=0)
    inputs = []
    previous = 0
    for tick in range(1, 5933):
        held = 1 if not previous else 0
        inputs.append({"kind": "input", "host_tick": tick, "held": held,
                       "pressed": held & ~previous})
        previous = held
    rows = inputs + [start]
    observations = [{"kind": "periodic", "host_tick": n * 1000, "frame": n * 993}
                    for n in range(1, 124)]
    observations.extend({"kind": "progress", "host_tick": 500 + n * 10000,
                         "frame": 400 + n * 990, "elapsed": 400 + n * 990,
                         "screen": 1, "won": 0, "lost": 0, "p3_outcome": 0,
                         "global_mission": 22, "selected_mission": 22, "race_mode": 0}
                        for n in range(12))
    rows.extend(sorted(observations, key=lambda row: row["host_tick"]))
    rows.append({"kind": "won", **final})
    return {"schema": 1, "mission": replay.MISSION_P3,
            "policy": replay.EXPANSION_POLICY, "events": rows}


def valid_stock_trace():
    """Synthetic bounded fixture matching the authenticated stock route's facts."""
    rows = []
    previous = 0
    input_ticks = list(range(1, 8))
    input_ticks.extend(8 + ((n - 8) * (58058 - 8)) // (2985 - 8)
                       for n in range(8, 2986))
    for index, tick in enumerate(input_ticks):
        held = (index & 1) + 1 if (index & 1) == 0 else 0
        # Alternate valid press/release transitions. Keep the recorded menu
        # prefix exactly equal to the authenticated stock trace.
        if index < len(replay.STOCK_OPENING_INPUTS):
            tick, held = replay.STOCK_OPENING_INPUTS[index]
        pressed = held & ~previous
        rows.append({"kind": "input", "host_tick": tick, "held": held, "pressed": pressed})
        previous = held
        if index == len(replay.STOCK_OPENING_INPUTS) - 1:
            rows.append({"kind": "start", **replay.STOCK_EXPECTED_START})

    periodic = []
    for n in range(1, 58):
        frame = n * 1000
        periodic.append({"kind": "periodic", "host_tick": frame + 32,
                         "frame": frame, "elapsed": frame, "screen": replay.SCREEN_PLAY,
                         "won": 0, "lost": 0, "p3_outcome": 0,
                         "p3_model_valid": 1, "campaign_phase": 1,
                         "objective_flags": 0})
    rows = rows[:len(replay.STOCK_OPENING_INPUTS) + 1] + sorted(
        rows[len(replay.STOCK_OPENING_INPUTS) + 1:] + periodic,
        key=lambda row: row["host_tick"])
    rows.append({"kind": "won", **replay.STOCK_EXPECTED_FINAL})
    return {"schema": 1, "mission": replay.MISSION_P3,
            "policy": replay.STOCK_POLICY, "events": rows}


class P3ControllerReplayValidation(unittest.TestCase):
    class DebuggerBytes:
        def __init__(self, data):
            self.data = data

        def read(self, address, size):
            return self.data[address:address + size]

    def test_signed_global_selection_value_is_decoded_as_minus_one(self):
        rsp = self.DebuggerBytes(b"\xff\xff\xff\xff")
        self.assertEqual(replay._debugger_scalar(rsp, 0, signed=True), -1)
        self.assertEqual(replay._debugger_scalar(rsp, 0, signed=False), 0xFFFFFFFF)

    def test_owner_and_selection_decoders_cover_legacy_and_compact_abis(self):
        for stride, offsets, widths, signed in (
                (108, {"alive": 0, "raw_owner": 4}, {"alive": 4, "raw_owner": 4},
                 {"alive": True, "raw_owner": True}),
                (48, {"alive": 34, "raw_owner": 22}, {"alive": 1, "raw_owner": 2},
                 {"alive": False, "raw_owner": True})):
            raw = bytearray(stride * 4)
            for index, (alive, owner) in enumerate(((1, -1), (1, 7), (0, 6), (1, 6))):
                base = index * stride
                raw[base + offsets["alive"]:base + offsets["alive"] + widths["alive"]] = alive.to_bytes(
                    widths["alive"], "little", signed=signed["alive"])
                raw[base + offsets["raw_owner"]:base + offsets["raw_owner"] + widths["raw_owner"]] = owner.to_bytes(
                    widths["raw_owner"], "little", signed=signed["raw_owner"])
            self.assertEqual(sum(replay._record_scalar(raw, i, stride, offsets, widths, signed, "alive") and
                                 replay._record_scalar(raw, i, stride, offsets, widths, signed, "raw_owner") == 7
                                 for i in range(4)), 1)
            self.assertEqual(sum(replay._record_scalar(raw, i, stride, offsets, widths, signed, "alive") and
                                 replay._record_scalar(raw, i, stride, offsets, widths, signed, "raw_owner") == 6
                                 for i in range(4)), 1)

        for stride, offsets, widths, signed in (
                (108, {"alive": 0, "selected": 4, "order": 8},
                 {"alive": 4, "selected": 4, "order": 4},
                 {"alive": True, "selected": True, "order": True}),
                (56, {"alive": 52, "selected": 53, "order": 54},
                 {"alive": 1, "selected": 1, "order": 1},
                 {"alive": False, "selected": False, "order": False})):
            raw = bytearray(stride * 4)
            for index, (alive, selected, order) in enumerate(((1, 1, 0), (1, 1, 8), (0, 1, 0), (1, 0, 0))):
                base = index * stride
                for name, value in (("alive", alive), ("selected", selected), ("order", order)):
                    width = widths[name]
                    raw[base + offsets[name]:base + offsets[name] + width] = value.to_bytes(
                        width, "little", signed=signed[name])
            self.assertEqual(replay._selected_unit_count(raw, 4, stride, offsets, widths, signed), 1)

    def test_queue_ring_decoder_handles_compact_and_legacy_widths(self):
        for stride, offsets, widths, signed, item_width in (
                (48, {"alive": 34, "owner": 32, "queue": 35, "queue_head": 40, "queue_count": 41},
                 {"alive": 1, "owner": 1, "queue_head": 1, "queue_count": 1},
                 {"alive": False, "owner": False, "queue_head": False, "queue_count": False}, 1),
                (100, {"alive": 24, "owner": 16, "queue": 28, "queue_head": 48, "queue_count": 52},
                 {"alive": 4, "owner": 4, "queue_head": 4, "queue_count": 4},
                 {"alive": True, "owner": True, "queue_head": True, "queue_count": True}, 4)):
            raw = bytearray(stride)
            for name, value in (("alive", 1), ("owner", 0), ("queue_head", 4), ("queue_count", 3)):
                width = widths[name]
                raw[offsets[name]:offsets[name] + width] = value.to_bytes(
                    width, "little", signed=signed[name])
            # Ring order 4,0,1 contains two Zealots and one other unit.
            for slot, value in enumerate((10, 4, 5, 0, 10)):
                start = offsets["queue"] + slot * item_width
                raw[start:start + item_width] = value.to_bytes(item_width, "little",
                                                                signed=(item_width == 4))
            self.assertEqual(replay._queued_type_count(
                raw, 1, stride, offsets, widths, signed, owner=0, unit_type=10), 2)
            raw[offsets["queue_head"]:offsets["queue_head"] + widths["queue_head"]] = (
                5).to_bytes(widths["queue_head"], "little", signed=signed["queue_head"])
            with self.assertRaisesRegex(RuntimeError, "queue count/head"):
                replay._queued_type_count(raw, 1, stride, offsets, widths, signed,
                                          owner=0, unit_type=10)

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

    def test_dense_window_capacity_uses_selected_scheduler_period(self):
        trace = valid_trace()
        trace["events"][5]["host_tick"] = 251
        plan = replay.validate_trace(trace, 3, windowed=True, window_ticks=250)
        self.assertEqual(plan["peak_page_transitions"], 3)
        with self.assertRaisesRegex(ValueError, "page exceeds"):
            replay.validate_trace(trace, 3, windowed=True, window_ticks=1000)

    def test_route_cli_defaults_to_stock_and_keeps_frozen_routes_explicit(self):
        self.assertEqual(replay.build_parser().parse_args([]).route, "stock")
        self.assertEqual(replay.DEFAULT_STOCK_TRACE,
                         replay.ROOT / ".local/sc64/p3-controller-stock-asan-win.json")
        self.assertEqual(replay.build_parser().parse_args(["--route", "garrison"]).route,
                         "garrison")
        self.assertEqual(replay.build_parser().parse_args(["--route", "expansion"]).route, "expansion")

    def test_expansion_route_is_additive_and_matches_its_own_witness(self):
        trace = valid_expansion_trace()
        plan = replay.validate_trace(trace, 6000, route="expansion")
        self.assertEqual(plan["route"], "expansion")
        self.assertEqual(plan["policy"], replay.EXPANSION_POLICY)
        self.assertEqual(plan["input_count"], 5932)
        self.assertEqual(plan["stop_tick"], 124689)
        # The default remains the original strict garrison contract.
        self.assertEqual(replay.validate_trace(valid_trace(), 4)["final"],
                         valid_trace()["events"][-1])

    def test_stock_route_is_additive_and_uses_its_authenticated_start_and_win(self):
        trace = valid_stock_trace()
        plan = replay.validate_trace(trace, 128, route="stock", windowed=True)
        self.assertEqual(plan["route"], "stock")
        self.assertEqual(plan["policy"], replay.STOCK_POLICY)
        self.assertEqual(plan["input_count"], 2985)
        self.assertEqual(plan["stop_tick"], 58063)
        self.assertEqual(plan["peak_page_transitions"], 58)
        self.assertEqual(replay.build_parser().parse_args(["--route", "stock"]).route, "stock")
        # Existing frozen routes retain their exact policy and stop ticks.
        self.assertEqual(replay.validate_trace(valid_trace(), 4)["stop_tick"], 152107)
        self.assertEqual(replay.validate_trace(valid_expansion_trace(), 6000,
                                               route="expansion")["stop_tick"], 124689)

    def test_stock_route_rejects_wrong_start_terminal_and_periodic_sequence(self):
        trace = valid_stock_trace()
        start = next(row for row in trace["events"] if row["kind"] == "start")
        start["force2_units"] -= 1
        with self.assertRaisesRegex(ValueError, "stock start force2_units"):
            replay.validate_trace(trace, 128, route="stock", windowed=True)

        trace = valid_stock_trace()
        trace["events"][-1]["primary_objective"] = 0
        with self.assertRaisesRegex(ValueError, "primary_objective"):
            replay.validate_trace(trace, 128, route="stock", windowed=True)

        trace = valid_stock_trace()
        periodic = next(row for row in trace["events"] if row["kind"] == "periodic")
        periodic["frame"] += 1
        with self.assertRaisesRegex(ValueError, "live stock-route checkpoint"):
            replay.validate_trace(trace, 128, route="stock", windowed=True)

    def test_stock_route_reuses_strict_transition_and_page_validation(self):
        trace = valid_stock_trace()
        trace["events"][1]["held"] = trace["events"][0]["held"]
        trace["events"][1]["pressed"] = 0
        with self.assertRaisesRegex(ValueError, "held-state transition"):
            replay.validate_trace(trace, 128, route="stock", windowed=True)

        trace = valid_stock_trace()
        trace["events"][1]["pressed"] = 1
        with self.assertRaisesRegex(ValueError, "pressed does not match"):
            replay.validate_trace(trace, 128, route="stock", windowed=True)

        trace = valid_stock_trace()
        with self.assertRaisesRegex(ValueError, "page exceeds"):
            replay.validate_trace(trace, 32, route="stock", windowed=True)
        with self.assertRaisesRegex(ValueError, "queue exceeds"):
            replay.validate_trace(trace, 128, route="stock", windowed=False)

    def test_expansion_rejects_wrong_route_policy_final_and_mission(self):
        trace = valid_expansion_trace()
        with self.assertRaisesRegex(ValueError, "policy"):
            replay.validate_trace(trace, 6000, route="garrison")
        bad = valid_expansion_trace()
        bad["events"][-1]["side_cannons"] += 1
        with self.assertRaisesRegex(ValueError, "side_cannons"):
            replay.validate_trace(bad, 6000, route="expansion")
        bad = valid_expansion_trace()
        bad["mission"] = 21
        with self.assertRaisesRegex(ValueError, "mission"):
            replay.validate_trace(bad, 6000, route="expansion")
        bad = valid_expansion_trace()
        bad["events"][-1]["global_mission"] = 21
        with self.assertRaisesRegex(ValueError, "global_mission"):
            replay.validate_trace(bad, 6000, route="expansion")

    def test_expansion_trace_requires_exact_verified_start(self):
        trace = valid_expansion_trace()
        next(row for row in trace["events"] if row["kind"] == "start")["raw_owner0_units"] += 1
        with self.assertRaisesRegex(ValueError, "raw_owner0_units"):
            replay.validate_trace(trace, 6000, route="expansion")

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
            "supply": 54, "supply_cap": 98, "gateways": 3,
            # SIDE wallet arrays are stale mirrors; globals are authoritative.
            "race_minerals": [300, 0, 0, 0, 0], "race_gas": [100, 0, 0, 0, 0],
            "race_campaign": {"mission": 22, "elapsed_frames": 151088, "phase": 3,
                              "objective_flags": 1},
            "race_p3": {"outcome": 1, "sequence": 5, "fired": 31,
                        "transmission": 0, "reserved": 0, "action_ticks": 0},
        }
        replay.verify_final(valid_trace()["events"][-1], state)
        # Trace gateways is a historical presence flag; the observer keeps the
        # actual live count, which can exceed one without invalidating the win.
        state["gateways"] = 1
        replay.verify_final(valid_trace()["events"][-1], state)
        state["gateways"] = 3
        stale_wallet_arrays = dict(state)
        stale_wallet_arrays["race_minerals"] = [999, 0, 0, 0, 0]
        stale_wallet_arrays["race_gas"] = [888, 0, 0, 0, 0]
        replay.verify_final(valid_trace()["events"][-1], stale_wallet_arrays)
        state["race_completed"] = 0
        with self.assertRaisesRegex(RuntimeError, "completion bit"):
            replay.verify_final(valid_trace()["events"][-1], state)

    def test_final_verifier_rejects_missing_gateway(self):
        state = {
            "screen": 3, "frame": 151088, "won": 1, "lost": 0,
            "side_units": 14, "side_buildings": 11,
            "force2_units": 7, "force2_buildings": 0,
            "force3_units": 0, "force3_buildings": 0,
            "minerals": 44, "gas": 1048, "race_completed": 1 << 22,
            "race_mode": 0, "race_selected_mission": 22,
            "supply": 54, "supply_cap": 98, "gateways": 0,
            "race_campaign": {"mission": 22, "elapsed_frames": 151088, "phase": 3,
                              "objective_flags": 1},
            "race_p3": {"outcome": 1, "sequence": 5, "fired": 31,
                        "transmission": 0, "reserved": 0, "action_ticks": 0},
        }
        with self.assertRaisesRegex(RuntimeError, "gateways presence"):
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

    def test_expansion_final_verifier_compares_witness_and_model(self):
        final = valid_expansion_trace()["events"][-1]
        state = {
            "screen": 3, "frame": 123916, "won": 1, "lost": 0,
            "side_units": 67, "side_buildings": 35,
            "force2_units": 3, "force2_buildings": 0,
            "force3_units": 1, "force3_buildings": 0,
            "minerals": 14840, "gas": 21088, "race_completed": 1 << 22,
            "race_mode": 0, "race_selected_mission": 22,
            "supply": 178, "supply_cap": 230, "gateways": 1,
            "mission_unsupported_count": 0,
            "raw_owner7_units": 1, "raw_owner0_units": 3, "raw_owner6_units": 1,
            "raw_owner7_buildings": 4, "raw_owner0_buildings": 0, "raw_owner6_buildings": 0,
            "side_army": 22, "side_probes": 45, "side_cannons": 11,
            "weapons_level": 2, "armor_level": 2, "weapons_research": 0,
            "armor_research": 0, "zealots_queued": 2, "selected_units": 0,
            "selected_building": 61, "geyser_count": 3,
            "race_campaign": {"mission": 22, "elapsed_frames": 123916, "phase": 3,
                              "objective_flags": 1},
            "race_p3": {"outcome": 1, "sequence": 5, "fired": 31,
                        "transmission": 0, "reserved": 0, "action_ticks": 0},
        }
        replay.verify_final(final, state, "expansion")
        for key, value in (("side_probes", 44), ("raw_owner6_units", 2),
                           ("selected_building", 62)):
            bad = dict(state)
            bad[key] = value
            with self.subTest(key=key), self.assertRaisesRegex(RuntimeError, key):
                replay.verify_final(final, bad, "expansion")

    def test_stock_final_verifier_compares_native_win_state_and_route_witness(self):
        final = valid_stock_trace()["events"][-1]
        state = {
            "screen": replay.SCREEN_RESULT, "frame": 57457, "won": 1, "lost": 0,
            "side_units": 22, "side_army": 22, "side_buildings": 4,
            "force2_units": 6, "force2_buildings": 0,
            "force3_units": 9, "force3_buildings": 0,
            "minerals": 491, "gas": 1890, "race_completed": 1 << 22,
            "race_mode": 0, "race_selected_mission": 22,
            "supply": 88, "supply_cap": 32, "gateways": 1,
            "zealots_queued": 0, "selected_units": 22,
            "selected_building": -1, "geyser_count": 3,
            "race_campaign": {"mission": 22, "elapsed_frames": 57457,
                              "phase": 3, "objective_flags": 1},
            "race_p3": {"outcome": 1, "sequence": 5, "fired": 31,
                        "transmission": 0, "reserved": 0, "action_ticks": 0},
        }
        replay.verify_final(final, state, "stock")
        for key, value in (("side_army", 21), ("selected_units", 21),
                           ("selected_building", 5), ("geyser_count", 2)):
            bad = dict(state)
            bad[key] = value
            with self.subTest(key=key), self.assertRaisesRegex(RuntimeError, key):
                replay.verify_final(final, bad, "stock")


if __name__ == "__main__":
    unittest.main()
