import copy
import sys
import unittest
from pathlib import Path
from unittest import mock

TOOLS = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS))

import sc64_zp_controller_replay as replay


def z7_trace(final_kind="won"):
    events = [{"kind": "input", "host_tick": tick, "held": held,
               "pressed": held if held else 0}
              for tick, held in replay.SEVENTH_MENU_PREFIX]
    starts = [0] * 8
    starts[1] = 1
    timers = [0] * 8
    timers[1] = 1
    events.append({"kind": "start", "host_tick": 12, "frame": 1,
                   "screen": 1, "won": 0, "lost": 0, "outcome": 0,
                   "started": 2, "fired": 2, "action_cursors": starts,
                   "action_ticks": timers, "switch10": 0, "ai_script_mask": 0,
                   "ai_plan_ticks": 0, "ai_plan_phase": 0, "fault": 0,
                   "side_units": 4, "side_buildings": 0, "enemy_units": 93,
                   "enemy_buildings": 57, "workers": 4})
    checkpoint = {"kind": "checkpoint", "host_tick": 300, "frame": 289,
                  "screen": 1, "won": 0, "lost": 0, "outcome": 0,
                  "started": 2, "fired": 2, "action_cursors": [0] * 8,
                  "action_ticks": [0] * 8, "switch10": 0, "ai_script_mask": 0,
                  "ai_plan_ticks": 0, "ai_plan_phase": 0, "fault": 0}
    events.append(checkpoint)
    started = 2 | (1 << 2) if final_kind == "won" else 2 | 1 if final_kind == "lost" else 2
    final = {"kind": final_kind, "host_tick": 400, "frame": 389,
             "screen": 3 if final_kind in ("won", "lost") else 1,
             "won": int(final_kind == "won"), "lost": int(final_kind == "lost"),
             "outcome": 1 if final_kind == "won" else 2 if final_kind == "lost" else 0,
             "started": started, "fired": started, "action_cursors": [0] * 8,
             "action_ticks": [0] * 8, "switch10": 1, "ai_script_mask": 0,
             "ai_plan_ticks": 0, "ai_plan_phase": 0, "fault": 0,
             "side_units": 0, "side_buildings": 1 if final_kind == "won" else 0,
             "enemy_units": 3, "enemy_buildings": 0 if final_kind == "won" else 4,
             "workers": 0}
    events.append(final)
    return {"schema": 1, "profile": "zerg-z7", "mission": 18,
            "policy": "synthetic Z7 replay fixture", "events": events}


class Z7TraceTests(unittest.TestCase):
    def test_seventh_entry_route_and_source_start_are_checked(self):
        plan = replay.validate_trace(z7_trace(), 240, "zerg-z7")
        self.assertEqual(plan["start"]["host_tick"], 12)
        self.assertEqual(plan["inputs"][:12], [(tick - 1, held)
                                                for tick, held in replay.SEVENTH_MENU_PREFIX])
        bad = z7_trace()
        bad["events"][6]["held"] = 128
        bad["events"][6]["pressed"] = 128
        with self.assertRaisesRegex(ValueError, "7th-entry deployment route"):
            replay.validate_trace(bad, 240, "zerg-z7")
        bad = z7_trace()
        bad["events"][12]["action_ticks"] = [0] * 8
        with self.assertRaisesRegex(ValueError, "cursor/timer"):
            replay.validate_trace(bad, 240, "zerg-z7")
        bad = z7_trace()
        # Trigger 4's first action is a 300-tick wait, but its second action
        # sets switch 10 and is a one-tick operation.
        bad["events"][12]["started"] |= 1 << 4
        bad["events"][12]["fired"] = bad["events"][12]["started"]
        bad["events"][12]["action_cursors"][4] = 2
        bad["events"][12]["action_ticks"][4] = 2
        with self.assertRaisesRegex(ValueError, "cursor/timer 4"):
            replay.validate_trace(bad, 240, "zerg-z7")

    def test_failed_trace_needs_explicit_bounded_attempt_prefix(self):
        failed = z7_trace("lost")
        with self.assertRaisesRegex(ValueError, "won event"):
            replay.validate_trace(failed, 240, "zerg-z7")
        plan = replay.validate_trace(failed, 240, "zerg-z7", attempt_prefix=True)
        self.assertEqual(plan["stop_tick"], 400)
        with self.assertRaisesRegex(ValueError, "only for zerg-z7"):
            replay.validate_trace(failed, 240, "zerg-z6", attempt_prefix=True)
        with self.assertRaisesRegex(ValueError, "failed or limited"):
            replay.validate_trace(z7_trace(), 240, "zerg-z7", attempt_prefix=True)

    def test_terminal_update_can_apply_the_last_input_transition(self):
        for kind in ("won", "lost", "limit"):
            with self.subTest(kind=kind):
                trace = z7_trace(kind)
                stop = trace["events"][-1]["host_tick"]
                trace["events"].insert(-1, {"kind": "input", "host_tick": stop,
                                           "held": 1, "pressed": 1})
                plan = replay.validate_trace(trace, 240, "zerg-z7",
                                             attempt_prefix=kind != "won")
                self.assertEqual(plan["inputs"][-1], (stop - 1, 1))
                self.assertEqual(plan["stop_tick"], stop)
                # A transition from a later update still cannot precede the
                # terminal observation, even with otherwise valid state fields.
                trace["events"][-2]["host_tick"] = stop + 1
                with self.assertRaisesRegex(ValueError, "unordered host_tick"):
                    replay.validate_trace(trace, 240, "zerg-z7",
                                          attempt_prefix=kind != "won")

    def test_terminal_must_follow_the_live_z7_start(self):
        trace = z7_trace("lost")
        trace["events"] = [row for row in trace["events"]
                           if row["kind"] != "checkpoint"]
        trace["events"][-1]["host_tick"] = 12
        with self.assertRaisesRegex(ValueError, "must follow the live start"):
            replay.validate_trace(trace, 240, "zerg-z7", attempt_prefix=True)

    def test_attempt_prefix_checks_only_replayed_queue_pages(self):
        failed = z7_trace("lost")
        # A dense failed suffix does not prevent observing the valid opening.
        tail = [{"kind": "input", "host_tick": tick, "held": (tick + 1) % 2,
                 "pressed": (tick + 1) % 2} for tick in range(320, 360)]
        failed["events"][-1:-1] = tail
        with self.assertRaisesRegex(ValueError, "event page exceeds"):
            replay.validate_trace(failed, 16, "zerg-z7", attempt_prefix=True,
                                  prefix_ticks=380, window_ticks=250)
        plan = replay.validate_trace(failed, 16, "zerg-z7", attempt_prefix=True,
                                     prefix_ticks=310, window_ticks=250)
        self.assertEqual(plan["peak_page_transitions"], 12)
        with self.assertRaisesRegex(ValueError, "before the failed/limited"):
            replay.validate_trace(failed, 240, "zerg-z7", attempt_prefix=True,
                                  prefix_ticks=400)

    def test_attempt_replay_forbids_save_before_opening_files_or_emulator(self):
        with mock.patch.object(replay.Path, "read_bytes") as read:
            with self.assertRaisesRegex(ValueError, "forbids --save-at-end"):
                replay.run(Path("trace"), Path("output"), Path("rom"), Path("elf"),
                           "zerg-z7", ticks=100, save_at_end=True,
                           attempt_prefix=True)
            read.assert_not_called()

    def test_limited_attempt_stays_live_and_runtime_faults_never_qualify(self):
        limited = z7_trace("limit")
        limited["events"][-1]["started"] = 2
        limited["events"][-1]["fired"] = 2
        replay.validate_trace(limited, 240, "zerg-z7", attempt_prefix=True,
                              prefix_ticks=310)
        limited["events"][-1]["outcome"] = 1
        with self.assertRaisesRegex(ValueError, "remain live and unfinished"):
            replay.validate_trace(limited, 240, "zerg-z7", attempt_prefix=True,
                                  prefix_ticks=310)
        bad = z7_trace()
        bad["events"][-1]["fault"] = 1
        with self.assertRaisesRegex(ValueError, "completed victory trigger"):
            replay.validate_trace(bad, 240, "zerg-z7")

    def test_full_win_allows_live_enemy_units_but_requires_zero_enemy_buildings(self):
        trace = z7_trace()
        self.assertEqual(trace["events"][-1]["enemy_units"], 3)
        replay.validate_trace(trace, 240, "zerg-z7")
        bad = copy.deepcopy(trace)
        bad["events"][-1]["enemy_buildings"] = 1
        with self.assertRaisesRegex(ValueError, "zero source enemy buildings"):
            replay.validate_trace(bad, 240, "zerg-z7")
        bad = copy.deepcopy(trace)
        bad["events"][-1]["side_buildings"] = 0
        with self.assertRaisesRegex(ValueError, "surviving player actor"):
            replay.validate_trace(bad, 240, "zerg-z7")
        bad = copy.deepcopy(trace)
        bad["events"][-1]["started"] |= 1
        bad["events"][-1]["fired"] = bad["events"][-1]["started"]
        with self.assertRaisesRegex(ValueError, "terminal cursors"):
            replay.validate_trace(bad, 240, "zerg-z7")
        bad = copy.deepcopy(trace)
        bad["events"][-1]["started"] |= 1 << 4
        bad["events"][-1]["fired"] = bad["events"][-1]["started"]
        bad["events"][-1]["action_cursors"][4] = 1
        bad["events"][-1]["action_ticks"][4] = 100
        with self.assertRaisesRegex(ValueError, "completed victory trigger"):
            replay.validate_trace(bad, 240, "zerg-z7")

    def test_z7_runtime_decoder_captures_all_persistent_fields_and_bounds(self):
        runtime_fields = {"model": 0, "action_ticks": 12, "switch10": 28,
                          "ai_script_mask": 29, "ai_plan_ticks": 30,
                          "ai_plan_phase": 32, "fault": 33}
        model_fields = {"started": 0, "outcome": 2, "action_cursor": 3}
        raw = bytearray(36)
        raw[0:2] = (2).to_bytes(2, "little")
        raw[2] = 0
        raw[3] = 0
        raw[12:14] = (1).to_bytes(2, "little")
        raw[14:28] = bytes(14)
        raw[28] = 1
        raw[29] = 0
        raw[30:32] = (0).to_bytes(2, "little")
        raw[32] = raw[33] = 0
        layout = (36, runtime_fields)
        model = (12, model_fields)
        decoded = replay.decode_runtime(bytes(raw), layout, model, "zerg-z7")
        self.assertEqual(decoded["model"], {"started": 2, "fired": 2,
                                             "outcome": 0, "action_cursor": [0] * 8})
        self.assertEqual(decoded["action_ticks"], [1] + [0] * 7)
        self.assertEqual(decoded["persistent_bytes"], list(raw))
        with self.assertRaisesRegex(ValueError, "runtime state bytes"):
            replay.decode_runtime(bytes(raw[:-1]), layout, model, "zerg-z7")
        bad = dict(runtime_fields, ai_plan_ticks=35)
        with self.assertRaisesRegex(ValueError, "ai_plan_ticks"):
            replay.decode_runtime(bytes(raw), (36, bad), model, "zerg-z7")

    def test_z7_persistent_layout_flattens_exact_nested_members(self):
        runtime = {"model": 0, "action_ticks": 12, "switch10": 28,
                   "ai_script_mask": 29, "ai_plan_ticks": 30,
                   "ai_plan_phase": 32, "fault": 33}
        with mock.patch.object(replay.races, "dwarf_struct", side_effect=lambda _elf, name:
                               (36, runtime) if name == "Sc64Z7RuntimeState" else
                               (36, {"runtime": 0})):
            size, fields = replay.runtime_layout_for(Path("fixture.elf"), "zerg-z7")
        self.assertEqual(size, 36)
        self.assertEqual(fields["fault"], 33)
        with mock.patch.object(replay.races, "dwarf_struct", side_effect=lambda _elf, name:
                               (37, runtime) if name == "Sc64Z7RuntimeState" else
                               (37, {"runtime": 0})):
            with self.assertRaisesRegex(RuntimeError, "36-byte source budget"):
                replay.runtime_layout_for(Path("fixture.elf"), "zerg-z7")

    def test_z7_compact_banks_use_240_units_and_160_buildings(self):
        unit = {"hp": 0, "x": 4, "y": 6, "raw_type": 8,
                "owner": 10, "type": 11, "alive": 12,
                "raw_owner": 13, "order": 14}
        building = {"x": 0, "y": 2, "hp": 4, "owner": 8,
                    "type": 9, "alive": 10, "under_construction": 11,
                    "raw_type": 12, "raw_owner": 13}

        def layout(_elf, name):
            if name == "Unit":
                return 56, unit
            if name == "Sc64ZpCompactBuilding":
                return 48, building
            raise AssertionError(name)

        with mock.patch.object(replay.races, "dwarf_struct", side_effect=layout):
            bank = replay.compact_bank_profile(
                Path("fixture.elf"), {"u": 240 * 56, "b": 160 * 48}, "zerg-z7")
        self.assertEqual((bank["unit_capacity"], bank["building_capacity"]), (240, 160))
        with mock.patch.object(replay.races, "dwarf_struct", side_effect=layout):
            with self.assertRaisesRegex(RuntimeError, "bounded profile"):
                replay.compact_bank_profile(
                    Path("fixture.elf"), {"u": 192 * 56, "b": 160 * 48}, "zerg-z7")
        for struct_name, missing, expected in (
                ("Unit", "raw_owner", "Z7 compact Unit DWARF lacks raw_owner"),
                ("Unit", "order", "Z7 compact Unit DWARF lacks order"),
                ("Sc64ZpCompactBuilding", "raw_owner",
                 "Z7 compact Building DWARF lacks raw_owner")):
            altered_unit, altered_building = dict(unit), dict(building)
            (altered_unit if struct_name == "Unit" else altered_building).pop(missing)

            def incomplete_layout(_elf, name):
                return (56, altered_unit) if name == "Unit" else (48, altered_building)

            with mock.patch.object(replay.races, "dwarf_struct", side_effect=incomplete_layout):
                with self.assertRaisesRegex(RuntimeError, expected):
                    replay.compact_bank_profile(
                        Path("fixture.elf"), {"u": 240 * 56, "b": 160 * 48}, "zerg-z7")

    def test_final_verification_uses_source_owner_censuses_and_allows_enemy_units(self):
        trace = z7_trace()["events"][-1]
        trace.update({"frame": 389, "started": 6, "fired": 6})
        runtime = {"model": {"outcome": 1, "started": 6, "fired": 6,
                             "action_cursor": [0] * 8},
                   "action_ticks": [0] * 8, "switch10": 1,
                   "ai_script_mask": 0, "ai_plan_ticks": 0, "ai_plan_phase": 0,
                   "fault": 0, "persistent_size": 34, "persistent_bytes": [0] * 34}
        state = {"screen": replay.SCREEN_RESULT, "frame": 389, "won": 1,
                 "lost": 0, "race_mode": 0, "race_selected_mission": 18,
                 "race_completed": 1 << 18,
                 "race_campaign": {"mission": 18, "elapsed_frames": 389,
                                   "phase": 3, "objective_flags": 1},
                 "race_runtime": runtime,
                 "units": {"entities": [
                     {"owner": 0, "raw_owner": 3, "raw_type": 42},
                     {"owner": 1, "raw_owner": 4, "raw_type": 37},
                     {"owner": 1, "raw_owner": 4, "raw_type": 37},
                     {"owner": 1, "raw_owner": 4, "raw_type": 38}]},
                 "buildings": {"entities": []}}
        trace["side_units"] = 1
        trace["side_buildings"] = 0
        trace["enemy_units"] = 3
        trace["enemy_buildings"] = 0
        trace["workers"] = 0
        replay.verify_final(trace, state, "zerg-z7")
        state["buildings"]["entities"] = [{"owner": 1, "raw_owner": 4, "raw_type": 131}]
        trace["enemy_buildings"] = 1
        with self.assertRaisesRegex(RuntimeError, "zero hostile buildings"):
            replay.verify_final(trace, state, "zerg-z7")


if __name__ == "__main__":
    unittest.main()
