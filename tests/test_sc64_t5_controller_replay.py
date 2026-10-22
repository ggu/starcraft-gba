"""Asset-free checks for the bounded T5 controller replay conversion."""
from __future__ import annotations

import importlib.util
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "sc64_t5_controller_replay", ROOT / "tools/sc64_t5_controller_replay.py")
assert SPEC and SPEC.loader
replay = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(replay)


OBS = {
    "frame": 0, "elapsed": 0, "screen": replay.SCREEN_PLAY,
    "race_mode": 0, "race_selected_mission": replay.MISSION_T5,
    "won": 0, "lost": 0, "faults": 0, "switch4": 0, "switch6": 0,
    "objective": 0, "side_units": 8, "enemy_units": 49,
    "side_buildings": 0, "enemy_buildings": 29, "kerrigan_alive": 1,
    "minerals": 0, "gas": 0, "enemy_minerals": 0, "enemy_gas": 0,
}


def observation(kind: str, tick: int, **updates: int) -> dict:
    frame = 0 if kind == "start" else tick
    row = {"kind": kind, "host_tick": tick, **OBS, "frame": frame, "elapsed": frame}
    row.update(updates)
    return row


def sample_trace() -> dict:
    events = [observation("start", 0)]
    # The frozen trace records a neutral sample at tick 1. This is valid but
    # has no transition to schedule; the normal menu route already releases A.
    events.append({"kind": "input", "host_tick": 1, "held": 0, "pressed": 0})
    events.append(observation("opening", 500, frame=500, elapsed=500, minerals=300, gas=300))
    events.append({"kind": "input", "host_tick": 501, "held": 1, "pressed": 1})
    events.append({"kind": "input", "host_tick": 505, "held": 0, "pressed": 0})
    events.append(observation("periodic", 1000, frame=1000, elapsed=1000))
    events.append(observation("command-center-approach-cleared", 1003,
                              frame=1003, elapsed=1003))
    events.append(observation("limit", 1200, frame=1200, elapsed=1200))
    return {"trace_schema": 1, "entry": "t5-probe-entry", "mission": 8,
            "policy": "bounded-normal-controller-quest-economy-ground-attack",
            "tick_limit": 1200, "events": events}


def add_aux_snapshot(row: dict, *, transport: bool = True) -> dict:
    row.update({
        "starport_slot": -1, "starport_raw": -1, "starport_complete": 0,
        "control_tower_slot": -1, "control_tower_raw": -1,
        "control_tower_complete": 0,
    })
    if transport:
        row.update({
            "transport_ship_slot": -1, "transport_ship_raw": -1,
            "transport_ship_alive": 0, "transport_ship_hp": -1,
            "transport_ship_xy": [-1, -1], "transport_cargo_space": -1,
            "transport_landing": [-1, -1], "transport_landed_count": -1,
            "transport_loaded": [],
        })
    return row


class T5ReplayValidationTests(unittest.TestCase):
    def test_source_trace_offsets_onto_verified_normal_menu_route(self):
        plan = replay.validate_trace(sample_trace(), capacity=256)
        self.assertEqual(plan["entry"], "campaign-menu")
        self.assertEqual(plan["start_tick"], 9)
        self.assertEqual(plan["stop_tick"], 1209)
        self.assertEqual(plan["start"]["host_tick"], 9)
        self.assertEqual(plan["start"]["frame"], 0)
        self.assertEqual(plan["input_count"], 12)
        self.assertEqual(plan["source_input_events"], 3)
        self.assertEqual(plan["source_input_count"], 2)
        self.assertEqual(plan["inputs"][:10], [(i, held) for i, held in enumerate(
            [replay.KEY_A, 0, replay.KEY_RIGHT, 0, replay.KEY_RIGHT,
             0, replay.KEY_A, 0, replay.KEY_A, 0])])
        self.assertEqual(plan["inputs"][10:], [(509, 1), (513, 0)])
        self.assertEqual(plan["checkpoints"][0]["host_tick"], 509)
        self.assertEqual(plan["checkpoints"][1]["host_tick"], 1009)
        self.assertEqual(plan["final"]["host_tick"], 1209)

    def test_exact_checkpoint_pages_and_nonmultiple_stop(self):
        plan = replay.validate_trace(sample_trace(), capacity=256)
        pages = replay.make_pages(plan, plan["stop_tick"], 256)
        ends = [page["end_tick"] for page in pages]
        self.assertEqual(ends, [9, 509, 1000, 1009, 1012, 1209])
        self.assertTrue(all(0 < page["end_tick"] - page["start_tick"] <= 1000
                            for page in pages))
        self.assertEqual(sum(len(page["events"]) for page in pages), plan["input_count"])
        self.assertEqual(pages[0]["events"], [(i, h) for i, h in enumerate(
            [replay.KEY_A, 0, replay.KEY_RIGHT, 0, replay.KEY_RIGHT,
             0, replay.KEY_A, 0, replay.KEY_A])])

    def test_prefix_ignores_future_events_but_keeps_exact_stop(self):
        plan = replay.validate_trace(sample_trace(), capacity=256)
        requested, prefix = replay.scheduler.run_length(plan, 1000)
        self.assertTrue(prefix)
        pages = replay.make_pages(plan, requested, 256)
        self.assertEqual(pages[-1]["end_tick"], 1000)
        self.assertEqual(sum(len(page["events"]) for page in pages), 12)
        with self.assertRaisesRegex(ValueError, "cannot exceed"):
            replay.scheduler.run_length(plan, 1210)

    def test_rejects_wrong_entry_or_mission_and_bad_start(self):
        for field, value in (("entry", "t5-probe"), ("mission", 7), ("trace_schema", 2)):
            trace = sample_trace()
            trace[field] = value
            with self.subTest(field=field), self.assertRaises(ValueError):
                replay.validate_trace(trace, 256)
        trace = sample_trace()
        trace["events"][0]["frame"] = 1
        with self.assertRaisesRegex(ValueError, "frame/elapsed zero"):
            replay.validate_trace(trace, 256)

    def test_rejects_malformed_input_transition_and_wrong_pressed_mask(self):
        trace = sample_trace()
        trace["events"][1]["pressed"] = 1
        with self.assertRaisesRegex(ValueError, "pressed"):
            replay.validate_trace(trace, 256)
        trace = sample_trace()
        trace["events"][3]["held"] = replay.scheduler.KEY_MASK + 1
        with self.assertRaisesRegex(ValueError, "held keys"):
            replay.validate_trace(trace, 256)
        trace = sample_trace()
        trace["events"][4]["host_tick"] = trace["events"][3]["host_tick"]
        with self.assertRaisesRegex(ValueError, "strictly increasing"):
            replay.validate_trace(trace, 256)

    def test_rejects_missing_checkpoint_fields_unsupported_kind_and_false_win(self):
        trace = sample_trace()
        del trace["events"][2]["enemy_buildings"]
        with self.assertRaisesRegex(ValueError, "missing required field"):
            replay.validate_trace(trace, 256)
        trace = sample_trace()
        trace["events"].insert(2, observation("mystery", 10))
        with self.assertRaisesRegex(ValueError, "unsupported kind"):
            replay.validate_trace(trace, 256)
        trace = sample_trace()
        trace["events"][-1]["kind"] = "won"
        with self.assertRaisesRegex(ValueError, "unfinished limit run"):
            replay.validate_trace(trace, 256)

    def test_rejects_overcapacity_observer_page(self):
        trace = sample_trace()
        # Nine menu transitions land before tick 9, so a too-small ELF queue is
        # rejected before any debugger/emulator process can be opened.
        plan = replay.validate_trace(trace, capacity=256)
        with self.assertRaisesRegex(ValueError, "exceeds ELF capacity"):
            replay.make_pages(plan, plan["stop_tick"], capacity=8)

    def test_unfinished_final_verification_never_claims_completion(self):
        trace = sample_trace()
        plan = replay.validate_trace(trace, capacity=256)
        state = dict(OBS, frame=1200, elapsed=1200, phase=1, race_mode=0,
                     race_selected_mission=8, screen=replay.SCREEN_PLAY,
                     human_outcome=0, outcomes=0, race_completed=0)
        state["race_campaign"] = {"mission": 8, "elapsed_frames": 1200, "phase": 1}
        state["won"] = state["lost"] = state["faults"] = 0
        state["race_mode"] = 0
        state["race_selected_mission"] = 8
        replay.verify_final(plan["final"], state)

    def test_accepts_optional_tech_and_transport_snapshots_without_comparing_them(self):
        trace = sample_trace()
        start = add_aux_snapshot(trace["events"][0])
        start.update({"starport_slot": 4, "starport_raw": 107,
                      "control_tower_slot": 5, "control_tower_raw": 113})
        loaded = add_aux_snapshot(observation("dropship-loaded", 1100))
        loaded.update({
            "starport_slot": 4, "starport_raw": 107, "starport_complete": 1,
            "control_tower_slot": 5, "control_tower_raw": 113,
            "control_tower_complete": 1,
            "transport_ship_slot": 12, "transport_ship_raw": 11,
            "transport_ship_alive": 1, "transport_ship_hp": 150,
            "transport_ship_xy": [900, 880], "transport_cargo_space": 8,
            "transport_landing": [1200, 700], "transport_landed_count": 0,
            "transport_loaded": [{"slot": 20, "raw_type": 16},
                                 {"slot": 21, "raw_type": 19}],
        })
        trace["events"].insert(-1, loaded)
        # The source transition trace remains valid and older fields continue
        # to be the only fields the replay debugger compares.
        plan = replay.validate_trace(trace, 256)
        self.assertEqual(plan["checkpoints"][-1]["kind"], "dropship-loaded")
        replay.compare_observation(loaded, dict(loaded), "fixture")
        self.assertTrue({"transport_loaded", "starport_complete"}.isdisjoint(
            replay.OBS_FIELDS))

    def test_accepts_destroyed_carrier_snapshot_and_new_checkpoint_kinds(self):
        trace = sample_trace()
        destroyed = add_aux_snapshot(observation("dropship-destroyed-in-transit", 1100))
        destroyed.update({
            "starport_slot": 4, "starport_raw": 107, "starport_complete": 1,
            "control_tower_slot": 5, "control_tower_raw": 113,
            "control_tower_complete": 1,
            "transport_ship_slot": 12, "transport_ship_raw": 11,
            "transport_ship_alive": 0, "transport_ship_hp": 0,
            "transport_ship_xy": [1000, 900], "transport_cargo_space": -1,
            "transport_landing": [1200, 700], "transport_landed_count": 0,
            "transport_loaded": [{"slot": 20, "raw_type": 16}],
        })
        trace["events"].insert(-1, destroyed)
        plan = replay.validate_trace(trace, 256)
        self.assertEqual(plan["checkpoints"][-1]["kind"],
                         "dropship-destroyed-in-transit")
        self.assertNotIn("dropship-destroyed-in-transit", replay.OBS_FIELDS)

    def test_rejects_partial_or_malformed_auxiliary_snapshots(self):
        trace = sample_trace()
        trace["events"][0]["transport_ship_slot"] = -1
        with self.assertRaisesRegex(ValueError, "snapshot is missing"):
            replay.validate_trace(trace, 256)

        for mutate, message in (
            (lambda row: row.update(transport_loaded=[{"slot": 1}]), "invalid shape"),
            (lambda row: row.update(transport_loaded=[{"slot": 1, "raw_type": 11},
                                                       {"slot": 1, "raw_type": 12}]), "duplicate slots"),
            (lambda row: row.update(transport_ship_xy=[1536, 0]), "map bounds"),
            (lambda row: row.update(transport_cargo_space=9), "capacity bounds"),
            (lambda row: row.update(starport_slot=64), "building bounds"),
        ):
            trace = sample_trace()
            row = add_aux_snapshot(trace["events"][0])
            mutate(row)
            with self.subTest(message=message), self.assertRaisesRegex(ValueError, message):
                replay.validate_trace(trace, 256)


if __name__ == "__main__":
    unittest.main()
