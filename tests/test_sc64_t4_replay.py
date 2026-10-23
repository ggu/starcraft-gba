#!/usr/bin/env python3
"""Asset-free contract tests for the private T4 ARM replay helper."""
from __future__ import annotations

import importlib.util
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "sc64_t4_controller_replay", ROOT / "tools/sc64_t4_controller_replay.py")
assert SPEC and SPEC.loader
replay = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(replay)


def trace(final_kind: str = "limit", entry: str = "internal-chord") -> dict:
    def observed(kind: str, tick: int, frame: int, screen: int, *, won: int = 0, lost: int = 0,
                 outcomes: int = 0, human_outcome: int = 0, objective: int = 0,
                 phase: int = 1) -> dict:
        return {"kind": kind, "host_tick": tick, "frame": frame, "elapsed": frame,
                "screen": screen, "won": won, "lost": lost, "race_mode": 0,
                "race_selected_mission": 7, "outcomes": outcomes, "faults": 0,
                "phase": phase,
                "switch4": 0, "switch7": 0, "objective": objective,
                "actor_slot": -1, "actor_alive": 0, "actor_x": -1, "actor_y": -1,
                "raynor_alive": 1, "human_outcome": human_outcome,
                "side_units": 12, "enemy_units": 5}

    if entry == "campaign-menu":
        held_states = [(1, replay.KEY_A), (2, 0), (3, replay.KEY_RIGHT), (4, 0),
                       (5, replay.KEY_DOWN), (6, 0), (7, replay.KEY_A), (8, 0),
                       (9, replay.KEY_A)]
        start_tick = 9
    else:
        held_states = [(1, replay.KEY_START | replay.KEY_SELECT), (2, 0),
                       (3, replay.KEY_A)]
        start_tick = 3
    rows = []
    previous_held = 0
    for tick, held in held_states:
        rows.append({"kind": "input", "host_tick": tick, "held": held,
                     "pressed": held & ~previous_held})
        previous_held = held
    rows.append(observed("start", start_tick, 0, replay.SCREEN_PLAY))
    if entry == "campaign-menu":
        rows.append({"kind": "input", "host_tick": 10, "held": 0, "pressed": 0})
    rows.append(observed("periodic", 1000, 1000 - start_tick, replay.SCREEN_PLAY))
    if final_kind == "won":
        rows.append(observed("won", 1001, 1001 - start_tick, replay.SCREEN_RESULT,
                             won=1, outcomes=1, human_outcome=1, objective=1, phase=3))
    elif final_kind == "lost":
        rows.append(observed("lost", 1001, 1001 - start_tick, replay.SCREEN_RESULT,
                             lost=1, outcomes=1, human_outcome=2, phase=4))
    else:
        rows.append(observed("limit", 1001, 1001 - start_tick, replay.SCREEN_PLAY))
    value = {"schema": 1, "mission": 7, "policy": "synthetic-controller-trace", "events": rows}
    if entry != "internal-chord":
        value["entry"] = entry
    return value


class ReplayValidationTests(unittest.TestCase):
    def test_report_profile_name_tracks_entry_route(self):
        self.assertEqual(replay.profile_label("internal-chord"), "terran-t4-internal")
        self.assertEqual(replay.profile_label("campaign-menu"), "terran-t4-campaign-menu")

    def test_valid_final_outcomes_and_checkpoint_alignment(self):
        for kind in ("won", "lost", "limit"):
            with self.subTest(kind=kind):
                plan = replay.validate_trace(trace(kind), capacity=256)
                self.assertEqual(plan["stop_tick"], 1001)
                self.assertEqual(plan["input_count"], 3)
                self.assertEqual(plan["entry"], "internal-chord")
                self.assertEqual(plan["start_tick"], 3)
                self.assertEqual(plan["peak_page_transitions"], 3)
                self.assertEqual(plan["final"]["kind"], kind)

    def test_final_can_share_tick_with_final_input(self):
        value = trace()
        value["events"] = value["events"][:4]
        final = dict(value["events"][3], kind="limit")
        value["events"].append(final)
        self.assertEqual(replay.validate_trace(value, 256)["stop_tick"], 3)

    def test_rejects_legacy_scheduler_wrong_mission_and_bad_launch_controls(self):
        with self.assertRaisesRegex(ValueError, "windowed"):
            replay.validate_trace(trace(), capacity=256, windowed=False)
        bad = trace()
        bad["mission"] = 6
        with self.assertRaisesRegex(ValueError, "mission"):
            replay.validate_trace(bad, capacity=256)
        bad = trace()
        bad["events"][0]["held"] = replay.KEY_START
        bad["events"][0]["pressed"] = replay.KEY_START
        with self.assertRaisesRegex(ValueError, "input prefix"):
            replay.validate_trace(bad, capacity=256)
        bad = trace()
        bad["events"][2]["held"] = replay.KEY_B if hasattr(replay, "KEY_B") else 2
        bad["events"][2]["pressed"] = bad["events"][2]["held"]
        with self.assertRaisesRegex(ValueError, "input prefix"):
            replay.validate_trace(bad, capacity=256)

    def test_campaign_menu_entry_has_exact_page_and_briefing_prefix(self):
        value = trace(entry="campaign-menu")
        plan = replay.validate_trace(value, capacity=256)
        self.assertEqual(plan["entry"], "campaign-menu")
        self.assertEqual(plan["start_tick"], 9)
        self.assertEqual(plan["input_count"], 10)  # nine navigation inputs and tick-10 release
        self.assertEqual(plan["start"]["host_tick"], 9)
        self.assertEqual(plan["start"]["frame"], 0)

        bad = trace(entry="campaign-menu")
        bad["entry"] = "arbitrary-mode"
        with self.assertRaisesRegex(ValueError, "entry must"):
            replay.validate_trace(bad, capacity=256)

        bad = trace(entry="campaign-menu")
        bad["events"][2]["held"] = 32  # LEFT instead of RIGHT; wrong page
        bad["events"][2]["pressed"] = 32
        with self.assertRaisesRegex(ValueError, "input prefix"):
            replay.validate_trace(bad, capacity=256)

        bad = trace(entry="campaign-menu")
        bad["events"][1]["held"] = replay.KEY_A  # omit required release after HOME A
        bad["events"][1]["pressed"] = 0
        with self.assertRaisesRegex(ValueError, "held-state transition"):
            replay.validate_trace(bad, capacity=256)

        bad = trace(entry="campaign-menu")
        bad["events"][9]["host_tick"] = 10
        with self.assertRaisesRegex(ValueError, "host_tick=9"):
            replay.validate_trace(bad, capacity=256)

    def test_rejects_inconsistent_terminal_labels_and_capacity_overflow(self):
        bad = trace("won")
        bad["events"][-1]["won"] = 0
        with self.assertRaisesRegex(ValueError, "won RESULT"):
            replay.validate_trace(bad, capacity=256)
        bad = trace("lost")
        bad["events"][-1]["phase"] = 3
        with self.assertRaisesRegex(ValueError, "phase contradicts"):
            replay.validate_trace(bad, capacity=256)
        bad = trace("limit")
        bad["events"][-1]["human_outcome"] = 2
        with self.assertRaisesRegex(ValueError, "limit event requires"):
            replay.validate_trace(bad, capacity=256)
        with self.assertRaisesRegex(ValueError, "capacity"):
            replay.validate_trace(trace(), capacity=2)
        with self.assertRaisesRegex(ValueError, "controller queue capacity"):
            replay.validate_trace(trace(), capacity=0)

    def test_requires_typed_launch_and_checkpoint_state_without_type_errors(self):
        bad = trace()
        del bad["events"][3]["race_selected_mission"]
        with self.assertRaisesRegex(ValueError, "race_selected_mission"):
            replay.validate_trace(bad, capacity=256)
        bad = trace()
        bad["events"][4]["frame"] = True
        with self.assertRaisesRegex(ValueError, "must be an integer"):
            replay.validate_trace(bad, capacity=256)
        bad = trace()
        del bad["events"][-1]["elapsed"]
        with self.assertRaisesRegex(ValueError, "elapsed"):
            replay.validate_trace(bad, capacity=256)

    def test_exact_stop_tick_is_not_a_prefix(self):
        self.assertFalse(replay.requested_is_prefix(1000, 1000))
        self.assertTrue(replay.requested_is_prefix(999, 1000))

    def test_decodes_all_eight_trigger_contexts_and_world_fields(self):
        trigger_offsets = {"elapsed_ticks": 0, "wait_ticks": 4, "cursor": 6, "flags": 70,
                           "owner": 134, "force": 135, "cooldown": 136, "outcome": 137,
                           "active_wait": 138, "fault": 139, "fault_trigger": 140,
                           "fault_slot": 141, "reserved": 142}
        offsets = {"player": 0, "switches": 1152, "location30": 1184,
                   "message_ticks": 1192, "ping_x": 1194, "ping_y": 1196,
                   "ping_ticks": 1198, "location30_moved": 1200, "message": 1201,
                   "active_mask": 1202, "doodad_slot": 1203, "doodad_enabled": 1235}
        raw = bytearray(1268)
        raw[trigger_offsets["owner"]:trigger_offsets["owner"] + 1] = bytes([0])
        raw[trigger_offsets["force"]:trigger_offsets["force"] + 1] = bytes([1])
        for owner in range(8):
            base = owner * 144
            raw[base + trigger_offsets["owner"]] = owner
            raw[base + trigger_offsets["force"]] = owner % 4 + 1
            raw[base + trigger_offsets["elapsed_ticks"]:base + trigger_offsets["elapsed_ticks"] + 4] = (100 + owner).to_bytes(4, "little")
        raw[1152] = 0x90
        raw[1184:1192] = b"\x01\x00\x02\x00\x03\x00\x04\x00"
        raw[1192:1203] = b"\x05\x00\x06\x00\x07\x00\x08\x00\x09\x0a\xdf"
        raw[1203:1235] = bytes(range(32))
        raw[1235:1267] = bytes([1] * 32)
        decoded = replay.decode_t4_state(bytes(raw), offsets, 144, trigger_offsets)
        self.assertEqual(len(decoded["players"]), 8)
        self.assertEqual(decoded["players"][7]["elapsed_ticks"], 107)
        self.assertEqual(decoded["players"][7]["owner"], 7)
        self.assertEqual(decoded["switches"][0], 0x90)
        self.assertEqual(decoded["location30"], [1, 2, 3, 4])
        self.assertEqual(decoded["message_ticks"], 5)
        self.assertEqual(decoded["ping"], {"x": 6, "y": 7, "ticks": 8})
        self.assertEqual(decoded["active_mask"], 0xDF)
        self.assertEqual(decoded["doodad_slot"], list(range(32)))
        self.assertEqual(decoded["doodad_enabled"], [1] * 32)
        with self.assertRaisesRegex(RuntimeError, "sizes"):
            replay.decode_t4_state(bytes(raw[:-1]), offsets, 144, trigger_offsets)

    def test_final_verifier_rejects_misreported_win(self):
        final = trace("won")["events"][-1]
        live = dict(final, screen=3, frame=998, elapsed=998, won=1, lost=0,
                    race_mode=0, race_selected_mission=7, race_campaign={
                        "mission": 7, "elapsed_frames": 998, "phase": 3,
                        "objective_flags": 1},
                    unit_slots=[])
        live["won"] = 0
        with self.assertRaisesRegex(RuntimeError, "won"):
            replay.verify_final(final, live)

    def test_final_win_does_not_assume_catalog_completion_bit(self):
        final = trace("won")["events"][-1]
        live = dict(final, screen=3, frame=998, elapsed=998, won=1, lost=0,
                    race_mode=0, race_selected_mission=7, race_completed=0,
                    race_campaign={"mission": 7, "elapsed_frames": 998, "phase": 3,
                                   "objective_flags": 1},
                    unit_slots=[])
        replay.verify_final(final, live)
        self.assertEqual(live["race_completed"], 0)

    def test_final_verifier_rejects_phase_outcome_and_primary_contradictions(self):
        final = trace("lost")["events"][-1]
        live = dict(final, screen=3, frame=998, elapsed=998, won=0, lost=1,
                    race_mode=0, race_selected_mission=7, human_outcome=2, faults=0,
                    race_campaign={"mission": 7, "elapsed_frames": 998, "phase": 4,
                                   "objective_flags": 0}, unit_slots=[])
        replay.verify_final(final, live)

        for campaign, label in [
            ({"mission": 7, "elapsed_frames": 998, "phase": 3, "objective_flags": 0}, "loss"),
            ({"mission": 7, "elapsed_frames": 998, "phase": 1, "objective_flags": 0}, "loss"),
        ]:
            bad = dict(live, race_campaign=campaign)
            with self.subTest(label=label, phase=campaign["phase"]), self.assertRaisesRegex(
                    RuntimeError, "loss phase"):
                replay.verify_final(final, bad)

    def test_dead_actor_coordinates_and_raynor_provenance(self):
        actual = {"unit_slots": [{"alive": 0, "x": 321, "y": 654}]}
        replay.compare_observation({"actor_slot": 0, "actor_alive": 0,
                                    "actor_x": 321, "actor_y": 654}, actual, "periodic")
        self.assertTrue(replay.raynor_alive_from_slots([
            {"alive": 1, "owner": 1, "raw_owner": 2, "raw_type": 20},
        ]))
        self.assertTrue(replay.raynor_alive_from_slots([
            {"alive": 1, "owner": 0, "raw_owner": 2, "raw_type": 20},
        ]))
        self.assertFalse(replay.raynor_alive_from_slots([
            {"alive": 0, "owner": 0, "raw_owner": 2, "raw_type": 20},
        ]))

    def test_reads_raw_owner_and_retained_coordinates_from_slot_bytes(self):
        class FakeRsp:
            def __init__(self, data):
                self.data = data

            def read(self, address, size):
                return self.data[address:address + size]

        fields = {"alive": 0, "owner": 4, "raw_owner": 8, "type": 12,
                  "raw_type": 16, "x": 20, "y": 24}
        first = bytearray(32)
        values = {"alive": 1, "owner": 1, "raw_owner": 2, "type": 20,
                  "raw_type": 20, "x": 1234, "y": 5678}
        for name, value in values.items():
            first[fields[name]:fields[name] + 4] = value.to_bytes(4, "little", signed=True)
        second = bytearray(32)
        second[fields["x"]:fields["x"] + 4] = (321).to_bytes(4, "little", signed=True)
        second[fields["y"]:fields["y"] + 4] = (654).to_bytes(4, "little", signed=True)
        bank = {"unit_offsets": fields, "unit_capacity": 2, "unit_stride_bytes": 32}
        slots = replay.read_unit_slots(FakeRsp(first + second), 0, bank)
        self.assertTrue(replay.raynor_alive_from_slots(slots))
        self.assertEqual((slots[1]["x"], slots[1]["y"]), (321, 654))


if __name__ == "__main__":
    unittest.main()
