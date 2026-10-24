import copy
import sys
import unittest
from unittest import mock
from pathlib import Path

TOOLS = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS))

import sc64_zp_controller_replay as replay


def trace_for(profile="zerg-z4", mission=15):
    return {
        "schema": 1, "mission": mission, "profile": profile,
        "policy": "asset-free fixture", "events": [
            {"kind": "input", "host_tick": 1, "held": 1, "pressed": 1},
            {"kind": "input", "host_tick": 2, "held": 0, "pressed": 0},
            {"kind": "input", "host_tick": 3, "held": 16, "pressed": 16},
            {"kind": "input", "host_tick": 4, "held": 0, "pressed": 0},
            {"kind": "input", "host_tick": 5, "held": 128, "pressed": 128},
            {"kind": "input", "host_tick": 6, "held": 0, "pressed": 0},
            {"kind": "input", "host_tick": 7, "held": 1, "pressed": 1},
            {"kind": "input", "host_tick": 8, "held": 0, "pressed": 0},
            {"kind": "input", "host_tick": 9, "held": 1, "pressed": 1},
            {"kind": "start", "host_tick": 9, "frame": 0, "screen": 1,
             "won": 0, "lost": 0, "outcome": 0},
            {"kind": "periodic", "host_tick": 1000},
            {"kind": "won", "host_tick": 2000, "frame": 1990,
             "screen": 3, "won": 1, "lost": 0, "outcome": 1, "fired": 3},
        ]}


class TraceTests(unittest.TestCase):
    class Memory:
        def __init__(self, data): self.data = data
        def read(self, address, size): return self.data[address:address + size]

    def test_compact_bank_profile_validates_layout_and_profile_bounds(self):
        unit = {"hp": 0, "x": 16, "y": 18, "raw_type": 32,
                "owner": 50, "type": 51, "alive": 52}
        building = {"x": 0, "y": 4, "hp": 8, "owner": 16, "type": 20,
                    "alive": 24, "under_construction": 64, "raw_type": 76}
        def layout(_elf, name):
            return (56, unit) if name == "Unit" else (100, building)
        with mock.patch.object(replay.races, "dwarf_struct", side_effect=layout):
            for name, capacity in (("zerg-z4", 192), ("protoss-p4", 240)):
                bank = replay.compact_bank_profile(Path("fixture.elf"),
                    {"u": capacity * 56, "b": 9600}, name)
                self.assertEqual(bank["unit_capacity"], capacity)
                self.assertEqual(bank["building_capacity"], 96)
            with self.assertRaisesRegex(RuntimeError, "bounded profile"):
                replay.compact_bank_profile(Path("fixture.elf"),
                    {"u": 128 * 56, "b": 9600}, "zerg-z4")
            building["alive"] = 99
            with self.assertRaisesRegex(RuntimeError, "bounded int field alive"):
                replay.compact_bank_profile(Path("fixture.elf"),
                    {"u": 192 * 56, "b": 9600}, "zerg-z4")

    def test_accepts_z4_and_p4_campaign_metadata(self):
        self.assertEqual(replay.validate_trace(trace_for(), 16, "zerg-z4")["stop_tick"], 2000)
        p4 = trace_for("protoss-p4", 23)
        self.assertEqual(replay.validate_trace(p4, 16, "protoss-p4")["input_count"], 9)

    def test_rejects_wrong_profile_mission_and_schema(self):
        p4_wrong = trace_for("protoss-p4", 15)
        with self.assertRaisesRegex(ValueError, "mission"):
            replay.validate_trace(p4_wrong, 16, "protoss-p4")
        bad = trace_for(); bad["mission"] = 14
        with self.assertRaisesRegex(ValueError, "global ID 15"):
            replay.validate_trace(bad, 16, "zerg-z4")
        bad = trace_for(); bad["schema"] = 2
        with self.assertRaisesRegex(ValueError, "schema"):
            replay.validate_trace(bad, 16, "zerg-z4")

    def test_p4_rescue_observation_preserves_input_schedule(self):
        trace = trace_for("protoss-p4", 23)
        baseline = replay.validate_trace(trace, 16, "protoss-p4")
        trace["events"].insert(-1, {"kind": "both-rescued", "host_tick": 1500})
        plan = replay.validate_trace(trace, 16, "protoss-p4")
        self.assertEqual(plan["inputs"], baseline["inputs"])
        self.assertEqual(plan["checkpoints"][-1]["kind"], "both-rescued")
        trace["profile"], trace["mission"] = "zerg-z4", 15
        with self.assertRaisesRegex(ValueError, "unsupported kind"):
            replay.validate_trace(trace, 16, "zerg-z4")

    def test_rejects_bad_input_edges_and_menu_deploy_order(self):
        bad = trace_for(); bad["events"][1]["host_tick"] = 1
        with self.assertRaisesRegex(ValueError, "strictly ordered"):
            replay.validate_trace(bad, 16, "zerg-z4")
        bad = trace_for(); bad["events"][0]["held"] = 0x400
        with self.assertRaisesRegex(ValueError, "0x3ff"):
            replay.validate_trace(bad, 16, "zerg-z4")
        bad = trace_for(); bad["events"][9]["host_tick"] = 10
        with self.assertRaisesRegex(ValueError, "launch at host_tick=9"):
            replay.validate_trace(bad, 16, "zerg-z4")
        bad = trace_for(); bad["events"][2]["held"] = 32; bad["events"][2]["pressed"] = 32
        with self.assertRaisesRegex(ValueError, "4th-entry deployment route"):
            replay.validate_trace(bad, 16, "zerg-z4")

    def test_rejects_unverified_or_malformed_terminal_result(self):
        bad = trace_for(); bad["events"][-1]["kind"] = "limit"
        with self.assertRaisesRegex(ValueError, "must end in a won"):
            replay.validate_trace(bad, 16, "zerg-z4")
        bad = trace_for(); bad["events"][-1]["outcome"] = 0
        with self.assertRaisesRegex(ValueError, "completed campaign-model win"):
            replay.validate_trace(bad, 16, "zerg-z4")
        bad = trace_for(); bad["events"].pop(-2); bad["events"][-1]["host_tick"] = 9
        with self.assertRaisesRegex(ValueError, "must follow"):
            replay.validate_trace(bad, 16, "zerg-z4")

    def test_windowed_capacity_is_checked_per_1000_tick_page(self):
        trace = trace_for()
        # Transition count beyond capacity is acceptable only when spread over
        # pages; one overfull page is rejected before any debugger launch.
        events = trace["events"]
        for i in range(20):
            held = 2 if i % 2 == 0 else 0
            events.insert(10 + i, {"kind": "input", "host_tick": 20 + i,
                               "held": held, "pressed": held})
        with self.assertRaisesRegex(ValueError, "event page exceeds"):
            replay.validate_trace(trace, 8, "zerg-z4")

    def test_runtime_model_decode_uses_compact_struct_field_widths(self):
        raw = bytearray(40)
        # Runtime.model starts at offset 2; Z4 model is 10 bytes: fired u16,
        # outcome u8, seven action cursor bytes.
        raw[2:12] = bytes([0x23, 0x01, 1, 1, 2, 3, 4, 5, 6, 7])
        # Seven u16 action timers, then u32 faults and single-byte fields.
        raw[12:18] = b"\0" * 6
        raw[18:20] = b"\0\0"
        decoded = replay.decode_runtime(bytes(raw),
            (40, {"model": 2, "countdown_ticks": 12, "action_ticks": 14,
                  "faults": 28, "message_ticks": 32, "switch_bits": 34,
                  "countdown_active": 35, "ai_script_mask": 36, "hatch_effects": 37}),
            (10, {"fired": 0, "outcome": 2, "action_cursor": 3}), "zerg-z4")
        self.assertEqual(decoded["model"], {"fired": 0x123, "outcome": 1,
                                             "action_cursor": [1, 2, 3, 4, 5, 6, 7]})

    def test_compact_unit_snapshot_reads_byte_owner_and_raw_type(self):
        row = bytearray(56)
        row[0:4] = (37).to_bytes(4, "little", signed=True)
        row[12:16] = (9).to_bytes(4, "little", signed=True)
        row[16:18] = (0x1234).to_bytes(2, "little", signed=True)
        row[18:20] = (-292).to_bytes(2, "little", signed=True)
        row[32:34] = (-1).to_bytes(2, "little", signed=True)
        row[34:36] = (-1).to_bytes(2, "little", signed=True)
        row[50:56] = bytes([4, 17, 1, 0, 2, 7])
        snapshot = replay.read_compact_units(self.Memory(row), 0, 1, 56,
            {"hp": 0, "shield": 12, "x": 16, "y": 18, "raw_type": 32,
             "raw_owner": 34, "owner": 50, "type": 51, "alive": 52,
             "selected": 53, "order": 54, "armor": 55})
        self.assertEqual(snapshot["by_owner"], {"4": 1})
        self.assertEqual(snapshot["entities"][0]["owner"], 4)
        self.assertEqual(snapshot["entities"][0]["type"], 17)
        self.assertEqual(snapshot["entities"][0]["raw_type"], -1)
        self.assertEqual(snapshot["entities"][0]["raw_owner"], -1)
        self.assertEqual(snapshot["entities"][0]["hp"], 37)
        self.assertEqual(snapshot["entities"][0]["shield"], 9)
        self.assertEqual(snapshot["entities"][0]["x"], 0x1234)
        self.assertEqual(snapshot["entities"][0]["y"], -292)

    def test_p4_optional_storm_observation_covers_all_effect_bytes(self):
        fields = {"model": 0, "action_ticks": 8, "message_ticks": 34,
                  "created_count": 36, "action_cursor": 38, "switch_bits": 51,
                  "source_paused": 52, "speech_muted": 53, "ai_script_mask": 54,
                  "switch13_set": 55, "faults": 56, "storm": 58}
        model = (8, {"fired": 0, "outcome": 2, "sequence_trigger": 3,
                     "action_index": 4, "sequence_active": 5, "reserved": 6})
        raw = bytes(58) + bytes(range(32))
        decoded = replay.decode_runtime(raw, (90, fields), model, "protoss-p4")
        self.assertEqual(decoded["storm"], list(range(32)))
        legacy = dict(fields); del legacy["storm"]
        self.assertNotIn("storm", replay.decode_runtime(
            raw[:58], (58, legacy), model, "protoss-p4"))
        fields["storm"] = 59
        with self.assertRaisesRegex(ValueError, "storm exceeds"):
            replay.decode_runtime(raw, (90, fields), model, "protoss-p4")

    def test_chrysalis_identity_counts_unit_and_building_banks(self):
        units = {"entities": [{"owner": 0, "raw_type": 150},
                              {"owner": 4, "raw_type": 150}]}
        buildings = {"entities": [{"owner": 0, "raw_type": 150}]}
        self.assertEqual(replay.count_chrysalis(units, buildings), 2)

    def test_final_verification_is_model_driven_and_profile_generic(self):
        final = trace_for()["events"][-1]
        state = {"screen": 3, "frame": 1990, "won": 1, "lost": 0, "race_mode": 0,
                 "race_selected_mission": 15, "race_completed": 1 << 15,
                 "race_campaign": {"mission": 15, "elapsed_frames": 1990,
                                   "phase": 3, "objective_flags": 1},
                 "race_runtime": {"model": {"outcome": 1, "fired": 3},
                                  "faults": 0, "hatch_effects": 3}}
        replay.verify_final(final, state, "zerg-z4")
        broken = copy.deepcopy(state); broken["race_runtime"]["hatch_effects"] = 1
        with self.assertRaisesRegex(RuntimeError, "incomplete hatch effects"):
            replay.verify_final(final, broken, "zerg-z4")
        broken = copy.deepcopy(state); broken["race_runtime"]["model"]["fired"] = 2
        with self.assertRaisesRegex(RuntimeError, "composite campaign model"):
            replay.verify_final(final, broken, "zerg-z4")

        p4_final = trace_for("protoss-p4", 23)["events"][-1]
        p4_final.update({"frame": 1990, "fired": 0x1010})
        p4_state = {"screen": 3, "frame": 1990, "won": 1, "lost": 0, "race_mode": 0,
                    "race_selected_mission": 23, "race_completed": 1 << 23,
                    "race_campaign": {"mission": 23, "elapsed_frames": 1990,
                                      "phase": 3, "objective_flags": 1},
                    "race_runtime": {"model": {"outcome": 1, "fired": 0x1010,
                                                 "sequence_active": 0, "sequence_trigger": 0,
                                                 "action_index": 0, "reserved": 0},
                                     "faults": 0},
                    "side_units": 4, "side_buildings": 2,
                    "force2_units": 0, "force3_units": 0}
        replay.verify_final(p4_final, p4_state, "protoss-p4")


    def test_p4_extraction_checks_both_real_hero_positions(self):
        final = {"escape": [100, 200, 160, 240]}
        actors = []
        for name, raw, x in (("tassadar", 79, 110), ("raynor", 19, 150)):
            final.update({f"{name}_alive": 1, f"{name}_owner": 0,
                          f"{name}_x": x, f"{name}_y": 220})
            actors.append({"raw_type": raw, "owner": 0, "x": x, "y": 220})
        state = {"units": {"entities": actors}}
        replay.verify_p4_hero_extraction(final, state)
        for raw in (19, 79):
            broken = copy.deepcopy(state)
            broken["units"]["entities"] = [a for a in actors if a["raw_type"] != raw]
            with self.assertRaisesRegex(RuntimeError, "exactly one live"):
                replay.verify_p4_hero_extraction(final, broken)
        for field, value in (("owner", 1), ("x", 160), ("y", 240)):
            wrong_final, wrong_state = copy.deepcopy(final), copy.deepcopy(state)
            wrong_final[f"raynor_{field}"] = value
            wrong_state["units"]["entities"][1][field] = value
            with self.assertRaisesRegex(RuntimeError, "inside extraction"):
                replay.verify_p4_hero_extraction(wrong_final, wrong_state)
        incomplete = copy.deepcopy(final); del incomplete["tassadar_y"]
        with self.assertRaisesRegex(RuntimeError, "complete tassadar"):
            replay.verify_p4_hero_extraction(incomplete, state)


if __name__ == "__main__":
    unittest.main()
