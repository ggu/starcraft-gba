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
            "transport_landed": [],
            "transport_passenger_snapshot_tick": row["host_tick"],
            "transport_passengers": [],
        })
    return row


def pool_unit(slot: int, raw: int, *, owner: int = 0, alive: int = 1,
              x: int = 512, y: int = 512, order: int = 0, target: int = -1,
              tx: int = -1, ty: int = -1, hp: int = 100) -> dict:
    return {"slot": slot, "raw_type": raw, "owner": owner, "alive": alive,
            "x": x, "y": y, "order": order, "target": target,
            "tx": tx, "ty": ty, "hp": hp}


class T5ReplayValidationTests(unittest.TestCase):
    def test_profile_entities_are_live_even_without_alive_member(self):
        rows = [{"raw_type": 16}, {"raw_type": 19, "alive": 0},
                {"raw_type": 20, "alive": 1}]
        self.assertEqual([replay._row_alive(row) for row in rows], [True, False, True])

    def test_live_optional_counts_come_from_actor_rows(self):
        rows = [pool_unit(0, 0, owner=0), pool_unit(1, 0, owner=0),
                pool_unit(2, 0, owner=0, order=replay._ORDER_TRANSPORTED),
                pool_unit(3, 8, owner=0), pool_unit(4, 8, owner=1),
                pool_unit(5, 19, owner=2), pool_unit(6, 19, owner=1, alive=0)]
        self.assertEqual(replay.live_controller_counts(rows), {
            "raynor_alive": 1, "marine_defenders": 2, "paid_wraith_alive": 1,
        })

    def test_untracked_host_slots_do_not_assert_world_absence(self):
        expected = add_aux_snapshot(observation("periodic", 500))
        expected.update({"starport_slot": -1, "starport_raw": -1,
                         "control_tower_slot": -1, "control_tower_raw": -1,
                         "transport_ship_slot": -1, "transport_ship_raw": -1})
        actual = {"replay_aux": {
            "starport_slot": 3, "starport_raw": 114, "starport_complete": 1,
            "control_tower_slot": 4, "control_tower_raw": 115, "control_tower_complete": 1,
            "transport_ship_slot": 12, "transport_ship_raw": 11,
            "transport_ship_alive": 1, "transport_ship_hp": 150,
            "transport_ship_xy": [400, 500], "transport_cargo_space": 0,
            "transport_landing": [-1, -1], "transport_landed_count": -1,
            "transport_loaded": [],
            "_building_rows": [
                {"slot": 3, "owner": 0, "raw_type": 114, "under_construction": 0},
                {"slot": 4, "owner": 0, "raw_type": 115, "under_construction": 0},
            ],
            "_unit_rows": [pool_unit(12, 11, x=400, y=500)],
        }}
        replay.compare_aux_observation(expected, actual, "untracked")
        expected["starport_slot"] = 2
        with self.assertRaisesRegex(RuntimeError, "starport identity/state mismatch"):
            replay.compare_aux_observation(expected, actual, "tracked")

    def test_replay_aux_reads_live_unit_pool_without_host_trace_symbols(self):
        class Rsp:
            def __init__(self, memory: dict[int, bytes]):
                self.memory = memory

            def read(self, address: int, count: int) -> bytes:
                data = self.memory[address]
                return data[:count]

        fields = {name: i * 4 for i, name in enumerate(
            ("x", "y", "hp", "owner", "type", "alive", "order", "tx", "ty", "target", "raw_type"))}
        stride, capacity, base = 44, 160, 0x1000
        pool = bytearray(stride * capacity)

        def put(slot: int, **values: int) -> None:
            for name, value in values.items():
                off = slot * stride + fields[name]
                pool[off:off + 4] = int(value).to_bytes(4, "little", signed=True)

        # The live ship target and cargo come entirely from game state. No
        # host trace_* globals or pre-recorded passenger arrays are consulted.
        put(12, x=800, y=900, hp=150, owner=0, type=11, alive=1,
            order=9, tx=664, ty=1336, target=-1, raw_type=11)
        put(20, x=800, y=900, hp=40, owner=0, type=0, alive=1,
            order=replay._ORDER_TRANSPORTED, tx=-1, ty=-1, target=12, raw_type=0)
        rsp = Rsp({base: bytes(pool)})
        bank = {"unit_capacity": capacity, "unit_stride_bytes": stride,
                "unit_offsets": fields}
        buildings = {"entities": [
            {"slot": 3, "owner": 0, "raw_type": 114, "under_construction": 0},
            {"slot": 4, "owner": 0, "raw_type": 115, "under_construction": 1},
        ]}
        aux = replay.replay_aux_state(rsp, {"u": base}, bank, buildings)
        self.assertEqual(aux["starport_slot"], 3)
        self.assertEqual(aux["starport_complete"], 1)
        self.assertEqual(aux["control_tower_complete"], 0)
        self.assertEqual(aux["transport_ship_slot"], 12)
        self.assertEqual(aux["transport_ship_xy"], [800, 900])
        self.assertEqual(aux["transport_landing"], [664, 1336])
        self.assertEqual(aux["transport_loaded"], [{"slot": 20, "raw_type": 0}])
        self.assertEqual(aux["transport_cargo_space"], 1)
        self.assertNotIn("transport_passengers", aux)

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

    def test_rejects_final_observation_beyond_declared_limit(self):
        trace = sample_trace()
        trace["events"][-1]["host_tick"] = trace["tick_limit"] + 1
        with self.assertRaisesRegex(ValueError, "final observation must match"):
            replay.validate_trace(trace, 256)

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

    def test_accepts_current_wraith_strategy_checkpoints_and_optional_actor_counts(self):
        trace = sample_trace()
        kinds = ("heroes-held-safe-home", "paid-supply-depot-complete",
                 "wraith-seed-isolated", "wraith-wing-gathering",
                 "wraith-wing-progress", "wraith-wing-before-attack",
                 "wraith-wing-attack-issued", "wraith-wing-after-attack",
                 "wraith-wing-assault")
        for index, kind in enumerate(kinds):
            tick = 510 + index
            trace["events"].append(observation(kind, tick, frame=tick, elapsed=tick,
                                               raynor_alive=1, marine_defenders=16,
                                               paid_wraith_alive=index))
        trace["events"].sort(key=lambda event: event["host_tick"])
        plan = replay.validate_trace(trace, 256)
        found = [row["kind"] for row in plan["checkpoints"] if row["kind"] in kinds]
        self.assertEqual(found, list(kinds))
        row = next(row for row in plan["checkpoints"] if row["kind"] == "wraith-wing-gathering")
        actual = {**OBS, "frame": row["frame"], "elapsed": row["elapsed"],
                  "raynor_alive": 1, "marine_defenders": 16, "paid_wraith_alive": 3}
        replay.compare_observation(row, actual, "wraith-counts")
        actual["paid_wraith_alive"] = 2
        with self.assertRaisesRegex(RuntimeError, "paid_wraith_alive mismatch"):
            replay.compare_observation(row, actual, "wraith-counts")

    def test_optional_actor_counts_are_typed_and_old_traces_may_omit_them(self):
        self.assertTrue(replay.validate_trace(sample_trace(), 256)["checkpoints"])
        trace = sample_trace()
        trace["events"][2]["marine_defenders"] = True
        with self.assertRaisesRegex(ValueError, "marine_defenders must be an integer"):
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
            "transport_ship_xy": [900, 880], "transport_cargo_space": 3,
            "transport_landing": [1200, 700], "transport_landed_count": 0,
            "transport_loaded": [{"slot": 20, "raw_type": 16},
                                 {"slot": 21, "raw_type": 19}],
            "transport_passengers": [
                {"slot": 20, "original_raw_type": 16, "alive": 1, "loaded": 1,
                 "xy": [900, 880]},
                {"slot": 21, "original_raw_type": 19, "alive": 1, "loaded": 1,
                 "xy": [900, 880]},
            ],
        })
        trace["events"].insert(-1, loaded)
        # The source transition trace remains valid and older fields continue
        # to be the only fields the replay debugger compares.
        plan = replay.validate_trace(trace, 256)
        self.assertEqual(plan["checkpoints"][-1]["kind"], "dropship-loaded")
        actual = dict(loaded)
        actual["replay_aux"] = {field: loaded[field] for field in replay.TECH_SNAPSHOT_FIELDS}
        actual["replay_aux"].update({field: loaded[field] for field in replay.TRANSPORT_SNAPSHOT_FIELDS
                                     if field in loaded})
        actual["replay_aux"].update({
            "_carrier_slot": 12,
            "_building_rows": [
                {"slot": 4, "owner": 0, "raw_type": 107, "under_construction": 0},
                {"slot": 5, "owner": 0, "raw_type": 113, "under_construction": 0},
            ],
            "_unit_rows": [pool_unit(12, 11, hp=150, x=900, y=880,
                                      order=9, tx=1200, ty=700),
                           pool_unit(20, 16, x=900, y=880, order=8, target=12),
                           pool_unit(21, 19, x=900, y=880, order=8, target=12)],
        })
        replay.compare_observation(loaded, actual, "fixture")
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
            "transport_passengers": [
                {"slot": 20, "original_raw_type": 16, "alive": 1, "loaded": 1,
                 "xy": [1000, 900]},
            ],
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
            (lambda row: row.update(transport_passengers=[{"slot": 0, "original_raw_type": 0,
                                                           "alive": 1, "loaded": 0,
                                                           "xy": [-1, -1]}]), "map bounds"),
        ):
            trace = sample_trace()
            row = add_aux_snapshot(trace["events"][0])
            mutate(row)
            with self.subTest(message=message), self.assertRaisesRegex(ValueError, message):
                replay.validate_trace(trace, 256)

    def test_compare_auxiliary_observations_checks_live_fields_and_frozen_passengers(self):
        expected = add_aux_snapshot(observation("dropship-loaded", 500))
        expected.update({
            "transport_ship_slot": 8, "transport_ship_raw": 11,
            "transport_ship_alive": 1, "transport_ship_hp": 150,
            "transport_ship_xy": [512, 512], "transport_cargo_space": 1,
            "transport_landing": [700, 800], "transport_landed_count": -1,
            "transport_loaded": [{"slot": 9, "raw_type": 0}],
            "transport_passenger_snapshot_tick": 500,
            "transport_passengers": [
                {"slot": 9, "original_raw_type": 0, "alive": 1, "loaded": 1,
                 "xy": [512, 512]},
            ],
        })
        actual = {"replay_aux": {field: expected[field] for field in replay.TECH_SNAPSHOT_FIELDS}}
        actual["replay_aux"].update({field: expected[field] for field in replay.TRANSPORT_SNAPSHOT_FIELDS})
        actual["replay_aux"].update({
            "_carrier_slot": 8,
            "_building_rows": [],
            "_unit_rows": [pool_unit(8, 11, hp=150, order=9, tx=700, ty=800),
                           pool_unit(9, 0, order=replay._ORDER_TRANSPORTED, target=8)],
        })
        replay.compare_aux_observation(expected, actual, "loaded")
        actual["replay_aux"]["_unit_rows"][1]["order"] = 0
        with self.assertRaisesRegex(RuntimeError, "transport_loaded mismatch"):
            replay.compare_aux_observation(expected, actual, "loaded")

        # The landing manifest is frozen at its stated source tick. It is not
        # compared against later live state, which may legitimately change.
        expected["kind"] = "periodic"
        expected["host_tick"] = 700
        expected["source_host_tick"] = 700
        expected["transport_passenger_snapshot_tick"] = 500
        actual["replay_aux"]["transport_loaded"] = expected["transport_loaded"]
        actual["replay_aux"]["transport_passengers"] = []
        actual["replay_aux"]["_unit_rows"] = [pool_unit(8, 11, hp=150, order=9,
                                                               tx=700, ty=800),
                                                pool_unit(9, 0, x=530, y=522,
                                                          order=replay._ORDER_TRANSPORTED,
                                                          target=8)]
        replay.compare_aux_observation(expected, actual, "later")

    def test_tracked_second_starport_and_carrier_use_exact_pool_slots(self):
        expected = add_aux_snapshot(observation("dropship-landed", 500))
        expected.update({
            "starport_slot": 7, "starport_raw": 114, "starport_complete": 1,
            "control_tower_slot": -1, "control_tower_raw": -1,
            "transport_ship_slot": 13, "transport_ship_raw": 11,
            "transport_ship_alive": 1, "transport_ship_hp": 120,
            "transport_ship_xy": [650, 700], "transport_cargo_space": 1,
            "transport_loaded": [{"slot": 21, "raw_type": 0}],
            "transport_landing": [750, 800], "transport_landed_count": 0,
            "transport_landed": [],
        })
        actual = {"replay_aux": {
            # The public summaries deliberately describe the first matching
            # actors; tracked identities must come from the decoded pools.
            "starport_slot": 2, "starport_raw": 114, "starport_complete": 1,
            "control_tower_slot": -1, "control_tower_raw": -1,
            "transport_ship_slot": 4, "transport_ship_raw": 11,
            "transport_ship_alive": 1, "transport_ship_hp": 150,
            "transport_ship_xy": [400, 500], "transport_cargo_space": 0,
            "transport_loaded": [],
            "_building_rows": [
                {"slot": 2, "owner": 0, "raw_type": 114, "under_construction": 0},
                {"slot": 7, "owner": 0, "raw_type": 114, "under_construction": 0},
            ],
            "_unit_rows": [
                pool_unit(4, 11, x=400, y=500, tx=1000, ty=1000),
                pool_unit(13, 11, x=650, y=700, hp=120, tx=750, ty=800),
                pool_unit(21, 0, x=650, y=700, order=replay._ORDER_TRANSPORTED,
                          target=13),
            ],
        }}
        replay.compare_aux_observation(expected, actual, "second-actors")

        # Away from a landed checkpoint the host value is the originally
        # planned point. The live carrier may later have a different target;
        # exact live-position checks must not reinterpret that frozen value.
        expected["kind"] = "periodic"
        expected["host_tick"] = 600
        expected["source_host_tick"] = 600
        expected["transport_passenger_snapshot_tick"] = 500
        actual["replay_aux"]["_unit_rows"][1]["tx"] = 1000
        actual["replay_aux"]["_unit_rows"][1]["ty"] = 1000
        replay.compare_aux_observation(expected, actual, "later-carrier-target")

        # A slot that exists but has the wrong owner/type must not fall back to
        # the first actor that happens to match the expected raw type.
        actual["replay_aux"]["_unit_rows"][1]["owner"] = 1
        with self.assertRaisesRegex(RuntimeError, "carrier identity mismatch"):
            replay.compare_aux_observation(expected, actual, "wrong-carrier")


if __name__ == "__main__":
    unittest.main()
