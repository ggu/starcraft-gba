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


def fifth_trace(profile="zerg-z5", mission=16):
    trace = trace_for(profile, mission)
    # The fifth catalog entry takes one more RIGHT before normal deployment.
    trace["events"][4]["held"] = 16
    trace["events"][4]["pressed"] = 16
    return trace


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

    def test_fifth_entry_requires_two_rights_and_accepts_z5_p5(self):
        for name, mission in (("zerg-z5", 16), ("protoss-p5", 24)):
            with self.subTest(profile=name):
                plan = replay.validate_trace(fifth_trace(name, mission), 16, name)
                self.assertEqual(plan["stop_tick"], 2000)
                self.assertEqual(plan["inputs"][:9], [
                    (0, 1), (1, 0), (2, 16), (3, 0), (4, 16),
                    (5, 0), (6, 1), (7, 0), (8, 1)])
                with self.assertRaisesRegex(ValueError, "5th-entry deployment route"):
                    replay.validate_trace(trace_for(name, mission), 16, name)

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

    def test_fifth_compact_building_decodes_signed_wide_fields_and_all_queue_bytes(self):
        fields = {"hp": 0, "maxhp": 4, "shield": 8, "build_progress": 12,
                  "x": 16, "y": 18, "raw_type": 20, "raw_owner": 22,
                  "queue_progress": 24, "flags": 26, "larva_timer": 28,
                  "shield_remainder": 30, "owner": 32, "type": 33,
                  "alive": 34, "queue_head": 35, "queue_count": 36,
                  "under_construction": 37, "level": 38, "larva": 39,
                  "queue": 40}
        row = bytearray(48)
        for name, value in (("hp", 100000), ("maxhp", 100000),
                            ("shield", -17), ("build_progress", 65534)):
            row[fields[name]:fields[name]+4] = value.to_bytes(4, "little", signed=True)
        for name, value in (("x", -23), ("y", 2047), ("raw_type", 196),
                            ("raw_owner", 2)):
            row[fields[name]:fields[name]+2] = value.to_bytes(2, "little", signed=True)
        for name, value in (("queue_progress", 65535), ("flags", 0xA55A),
                            ("larva_timer", 0x9ABC), ("shield_remainder", 0x1234)):
            row[fields[name]:fields[name]+2] = value.to_bytes(2, "little")
        row[32:40] = bytes([0, 12, 1, 4, 5, 1, 2, 3])
        row[40:45] = bytes([9, 8, 7, 6, 5])
        snap = replay.read_compact_buildings(self.Memory(bytes(row)), 0, 1, 48, fields)
        entity = snap["entities"][0]
        self.assertEqual(entity["hp"], 100000)
        self.assertEqual(entity["maxhp"], 100000)
        self.assertEqual(entity["shield"], -17)
        self.assertEqual(entity["build_progress"], 65534)
        self.assertEqual((entity["x"], entity["y"], entity["raw_type"],
                          entity["raw_owner"]), (-23, 2047, 196, 2))
        self.assertEqual((entity["queue_progress"], entity["flags"],
                          entity["larva_timer"], entity["shield_remainder"]),
                         (65535, 0xA55A, 0x9ABC, 0x1234))
        self.assertEqual(entity["queue"], [9, 8, 7, 6, 5])
        bad = dict(fields); bad["queue"] = 44
        with self.assertRaisesRegex(RuntimeError, "queue exceeds or is absent"):
            replay.read_compact_buildings(self.Memory(bytes(row)), 0, 1, 48, bad)

    def test_fifth_snapshot_hashes_complete_banks_and_reads_unit_carry_maxhp(self):
        stride = 56
        row = bytearray(stride)
        row[0:4] = (1234).to_bytes(4, "little", signed=True)
        row[4:8] = (2500).to_bytes(4, "little", signed=True)
        row[8:12] = (500).to_bytes(4, "little", signed=True)
        row[16:18] = (100).to_bytes(2, "little", signed=True)
        row[18:20] = (200).to_bytes(2, "little", signed=True)
        row[32:34] = (79).to_bytes(2, "little", signed=True)
        row[34:36] = (2).to_bytes(2, "little", signed=True)
        row[50:56] = bytes([0, 8, 1, 0, 0, 0])
        building = bytearray(48)
        building[0:4] = (5000).to_bytes(4, "little", signed=True)
        building[4:8] = (5000).to_bytes(4, "little", signed=True)
        building[16:18] = (400).to_bytes(2, "little", signed=True)
        building[18:20] = (500).to_bytes(2, "little", signed=True)
        building[20:22] = (106).to_bytes(2, "little", signed=True)
        building[22:24] = (7).to_bytes(2, "little", signed=True)
        building[32:40] = bytes([0, 0, 1, 0, 0, 0, 0, 0])
        memory = bytearray(320)
        memory[0:56] = row
        memory[100:148] = building
        memory[250:270] = bytes(20)
        memory[270:290] = bytes(20)
        rsp = self.Memory(bytes(memory))
        unit_offsets = {"hp":0,"maxhp":4,"carry":8,"x":16,"y":18,
                        "raw_type":32,"raw_owner":34,"owner":50,
                        "type":51,"alive":52,"selected":53,"order":54,"armor":55}
        building_offsets = {"hp":0,"maxhp":4,"shield":8,"build_progress":12,
                            "x":16,"y":18,"raw_type":20,"raw_owner":22,
                            "queue_progress":24,"flags":26,"larva_timer":28,
                            "shield_remainder":30,"owner":32,"type":33,"alive":34,
                            "queue_head":35,"queue_count":36,"under_construction":37,
                            "level":38,"larva":39,"queue":40}
        sym = {"u":0,"b":100,"race_campaign":150,"race_z5":200,
               "race_minerals":250,"race_gas":270}
        bank = {"unit_capacity":1,"building_capacity":1,"unit_stride_bytes":56,
                "building_stride_bytes":48,"unit_offsets":unit_offsets,
                "building_offsets":building_offsets}
        with mock.patch.object(replay.profile, "u32", return_value=0), \
             mock.patch.object(replay.races, "campaign_state", return_value={}), \
             mock.patch.object(replay, "decode_runtime", return_value={"persistent_bytes":[1]}):
            snapshot = replay.mission_snapshot(rsp, sym, {"u":56,"b":48,
                "race_minerals":20,"race_gas":20}, bank, (0,{}), (1,{}),
                (1,{}), "zerg-z5")
        self.assertEqual(snapshot["units"]["entities"][0]["carry"], 500)
        self.assertEqual(snapshot["units"]["entities"][0]["maxhp"], 2500)
        self.assertEqual(snapshot["units"]["entities"][0]["raw_owner"], 2)
        self.assertEqual(snapshot["unit_bank_sha256"], replay.hashlib.sha256(bytes(row)).hexdigest())
        self.assertEqual(snapshot["building_bank_sha256"], replay.hashlib.sha256(bytes(building)).hexdigest())

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

    def test_nested_z5_persistent_runtime_flattens_and_decodes_every_saved_field(self):
        inner = (172, {"model": 0, "action_ticks": 52, "switch_bits": 132,
                       "source_paused": 164, "fault": 165})
        outer = (181, {"runtime": 0, "doodad_enabled": 172,
                       "pause_acknowledged": 178, "cloak": 179})
        def dwarf(_elf, name):
            if name == "Sc64Z5RuntimeState": return inner
            if name == "Sc64Z5PersistentState": return outer
            raise AssertionError(name)
        with mock.patch.object(replay.races, "dwarf_struct", side_effect=dwarf):
            layout = replay.runtime_layout_for(Path("fixture.elf"), "zerg-z5")
        self.assertEqual(layout[0], 181)
        self.assertEqual(layout[1]["action_ticks"], 52)
        self.assertEqual(layout[1]["doodad_enabled"], 172)
        self.assertEqual(layout[1]["cloak"], 179)

        model = (49, {"outcome": 0, "action_cursor": 1,
                      "started_lo": 41, "started_hi": 45})
        raw = bytearray(181)
        raw[0] = 1
        raw[1:41] = bytes(range(40))
        raw[41:45] = (0x89ABCDEF).to_bytes(4, "little")
        raw[45:49] = (0x01234567).to_bytes(4, "little")
        for i in range(40):
            raw[52+2*i:54+2*i] = (1000+i).to_bytes(2, "little")
        for i in range(8):
            raw[132+4*i:136+4*i] = (0x11111111*i).to_bytes(4, "little")
        raw[164] = 1
        raw[165] = 0
        raw[172:178] = bytes([1, 2, 3, 4, 5, 6])
        raw[178] = 1
        raw[179:181] = bytes([1, 4])
        decoded = replay.decode_runtime(bytes(raw), layout, model, "zerg-z5")
        self.assertEqual(decoded["model"]["action_cursor"], list(range(40)))
        self.assertEqual(decoded["model"]["fired"], 0x0123456789ABCDEF)
        self.assertEqual(decoded["action_ticks"], list(range(1000, 1040)))
        self.assertEqual(decoded["switch_bits"], [0x11111111*i for i in range(8)])
        self.assertEqual(decoded["source_paused"], 1)
        self.assertEqual(decoded["doodad_enabled"], [1, 2, 3, 4, 5, 6])
        self.assertEqual(decoded["pause_acknowledged"], 1)
        self.assertEqual(decoded["cloak"], [1, 4])
        self.assertEqual(decoded["persistent_bytes"], list(raw))

    def test_nested_p5_persistent_decode_keeps_deaths_pressure_and_padding(self):
        inner = (71, {"model": 0, "action_ticks": 16, "switch_bits": 38,
                      "fault": 70})
        outer = (123, {"runtime": 0, "deaths_by_source": 71,
                       "pressure_dispatched_mask": 119, "reserved": 120})
        def dwarf(_elf, name):
            if name == "Sc64P5RuntimeState": return inner
            if name == "Sc64P5PersistentState": return outer
            raise AssertionError(name)
        with mock.patch.object(replay.races, "dwarf_struct", side_effect=dwarf):
            layout = replay.runtime_layout_for(Path("fixture.elf"), "protoss-p5")
        model = (14, {"outcome": 0, "action_cursor": 1, "started": 12})
        raw = bytearray(123)
        raw[0] = 1
        raw[1:12] = bytes(range(11))
        raw[12:14] = (0x07F3).to_bytes(2, "little")
        for i in range(11): raw[16+2*i:18+2*i] = (300+i).to_bytes(2, "little")
        for i in range(8): raw[38+4*i:42+4*i] = (0xA0000000+i).to_bytes(4, "little")
        for i in range(12): raw[71+4*i:75+4*i] = (100+i).to_bytes(4, "little")
        raw[119] = 5
        raw[120:123] = bytes([0, 1, 2])
        decoded = replay.decode_runtime(bytes(raw), layout, model, "protoss-p5")
        self.assertEqual(decoded["model"]["action_cursor"], list(range(11)))
        self.assertEqual(decoded["model"]["fired"], 0x07F3)
        self.assertEqual(decoded["action_ticks"], list(range(300, 311)))
        self.assertEqual(decoded["switch_bits"], [0xA0000000+i for i in range(8)])
        self.assertEqual(decoded["deaths_by_source"], list(range(100, 112)))
        self.assertEqual(decoded["pressure_dispatched_mask"], 5)
        self.assertEqual(decoded["reserved"], [0, 1, 2])
        self.assertEqual(decoded["persistent_bytes"], list(raw))

    def test_fifth_final_verification_checks_p5_provenance_unloaded_state_and_faults(self):
        profiles = (("zerg-z5", 16, 40, 0x0123456789ABCDEF),
                    ("protoss-p5", 24, 11, 2012))
        for profile_name, mission, cursor_count, fired in profiles:
            final = {"frame": 200, "fired": fired,
                     "action_cursors": [0] * cursor_count,
                     "action_ticks": [0] * cursor_count}
            runtime = {"model": {"outcome": 1, "fired": fired,
                                  "action_cursor": [0] * cursor_count},
                       "action_ticks": [0] * cursor_count, "faults": 0}
            state = {"screen": 3, "frame": 200, "won": 1, "lost": 0,
                     "race_mode": 0, "race_selected_mission": mission,
                     "race_completed": 1 << mission,
                     "race_campaign": {"mission": mission, "elapsed_frames": 200,
                                       "phase": 3, "objective_flags": 1},
                     "race_runtime": runtime, "kerrigan_alive": 1}
            if profile_name == "protoss-p5":
                final.update({"goal": [1696, 1680, 1808, 1776],
                              "hero": [1, 1704, 1772, 0], "zealots_at_goal": 2,
                              "source_deaths": [0] * 12, "pressure_dispatched": 0})
                runtime.update({"deaths_by_source": [0] * 12,
                                "pressure_dispatched_mask": 0})
                state["units"] = {"entities": [
                    {"raw_type": 79, "raw_owner": 2, "owner": 0, "order": 0,
                     "x": 1704, "y": 1772},
                    {"raw_type": 65, "raw_owner": 7, "owner": 0, "order": 0,
                     "x": 1702, "y": 1770},
                    {"raw_type": 65, "raw_owner": 7, "owner": 0, "order": 0,
                     "x": 1706, "y": 1774}]}
                replay.verify_p5_extraction(final, state)
                wrong_origin = copy.deepcopy(state)
                wrong_origin["units"]["entities"][1]["raw_owner"] = 6
                with self.assertRaisesRegex(RuntimeError, "Zealots"):
                    replay.verify_p5_extraction(final, wrong_origin)
                wrong_hero_origin = copy.deepcopy(state)
                wrong_hero_origin["units"]["entities"][0]["raw_owner"] = 7
                with self.assertRaisesRegex(RuntimeError, "Tassadar"):
                    replay.verify_p5_extraction(final, wrong_hero_origin)
                loaded = copy.deepcopy(state)
                loaded["units"]["entities"][2]["order"] = 8
                with self.assertRaisesRegex(RuntimeError, "Zealots"):
                    replay.verify_p5_extraction(final, loaded)
                wrong_hero = copy.deepcopy(final)
                wrong_hero["hero"] = [1, 0, 1704, 1772]
                with self.assertRaisesRegex(RuntimeError, "trace"):
                    replay.verify_p5_extraction(wrong_hero, state)
            replay.verify_final(final, state, profile_name)
            bad_cursor = copy.deepcopy(state)
            bad_cursor["race_runtime"]["model"]["action_cursor"][0] = 1
            with self.assertRaisesRegex(RuntimeError, "action cursors"):
                replay.verify_final(final, bad_cursor, profile_name)
            bad_ticks = copy.deepcopy(state)
            bad_ticks["race_runtime"]["action_ticks"][0] = 1
            with self.assertRaisesRegex(RuntimeError, "wait timers"):
                replay.verify_final(final, bad_ticks, profile_name)
            bad_fault = copy.deepcopy(state)
            bad_fault["race_runtime"]["faults"] = 1
            with self.assertRaisesRegex(RuntimeError, "adapter faults"):
                replay.verify_final(final, bad_fault, profile_name)
            if profile_name == "protoss-p5":
                bad_deaths = copy.deepcopy(state)
                bad_deaths["race_runtime"]["deaths_by_source"][2] = 1
                with self.assertRaisesRegex(RuntimeError, "death counters"):
                    replay.verify_final(final, bad_deaths, profile_name)
                bad_pressure = copy.deepcopy(state)
                bad_pressure["race_runtime"]["pressure_dispatched_mask"] = 1
                with self.assertRaisesRegex(RuntimeError, "pressure state"):
                    replay.verify_final(final, bad_pressure, profile_name)

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
