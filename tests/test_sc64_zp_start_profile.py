import copy
import sys
import unittest
from pathlib import Path

TOOLS = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS))

import sc64_zp_start_profile as startup


def catalog():
    return {"profile": "protoss", "enabled_mission_ids": [23, 24, 25],
            "records": [{"id": 25, "faction": 2, "chapter": 6, "playable": 1}]}


def symbols():
    names = startup.P6_REQUIRED_SYMBOLS + (
        "demo_schedule_active", "demo_schedule_index")
    return {name: i * 4 for i, name in enumerate(names, 1)}


def sizes():
    return {"demo_schedule_events": 16 * 8,
            "demo_schedule_active": 4, "demo_schedule_index": 4}


def state():
    cursors = [0] * 26
    cursors[0] = cursors[23] = 1
    ticks = [0] * 26
    ticks[0] = ticks[23] = 1
    return {"screen": 1, "race_mode": 0, "race_selected_mission": 25,
            "won": 0, "lost": 0, "frame": 1,
            "race_runtime": {"fault": 0, "model": {"outcome": 0,
                "started": 0x800001, "fired": 0x800001, "action_cursor": cursors},
                "action_ticks": ticks}}


def report(live=1000, interval=1000, *, state_value=None, capture_hash="a" * 64):
    return {"schema": 1, "kind": "p6-startup-diagnostic", "qualification": "startup-only",
            "profile": "protoss-p6", "mission": 25,
            "live_frames": live, "checkpoint_interval": interval,
            "rom_sha256": "1" * 64, "elf_sha256": "2" * 64,
            "start_state": state_value or state(),
            "start_capture_sha256": capture_hash,
            "checkpoints": [{"scheduler_tick": 1012, "live_frame": 1000,
                             "state": state_value or state(), "capture_sha256": capture_hash}]}


def z6_catalog():
    missions = (0, 1, 14, 15, 16, 17)
    return {"profile": "zerg", "enabled_mission_ids": list(missions),
            "records": [{"id": mission, "faction": 1, "chapter": chapter,
                         "playable": 1} for chapter, mission in enumerate(missions, 1)]}


def z6_symbols():
    names = startup.Z6_REQUIRED_SYMBOLS + ("demo_schedule_active", "demo_schedule_index")
    return {name: i * 4 for i, name in enumerate(names, 1)}


def z6_state():
    return {"screen": 1, "race_mode": 0, "race_selected_mission": 17,
            "won": 0, "lost": 0, "frame": 1,
            "race_runtime": {"fault": 0, "persistent_size": 176, "persistent_bytes": [0] * 176,
                "human_drone_morphs": 0, "ai_plan_ticks": 0, "ai_plan_phase": 0,
                "moved_location": [0] * 8, "transport": [0, 0, 0],
                "model": {"outcome": 0, "started": 1, "fired": 1,
                          "action_cursor": [1,0,0,0,0,0,0,0,0]},
                "action_ticks": [1,0,0,0,0,0,0,0,0]}}


def z6_report(live=1000, interval=1000, *, state_value=None, capture_hash="a" * 64):
    return {"schema": 1, "kind": "zerg-z6-startup-diagnostic", "qualification": "startup-only",
            "profile": "zerg-z6", "mission": 17,
            "live_frames": live, "checkpoint_interval": interval,
            "rom_sha256": "1" * 64, "elf_sha256": "2" * 64,
            "start_state": state_value or z6_state(),
            "start_capture_sha256": capture_hash,
            "checkpoints": [{"scheduler_tick": 1012, "live_frame": 1000,
                             "state": state_value or z6_state(), "capture_sha256": capture_hash}]}


class StartupProfileTests(unittest.TestCase):
    def test_default_scope_and_menu_prefix_are_exact(self):
        self.assertEqual(startup.DEFAULT_LIVE_FRAMES, 1000)
        self.assertEqual(startup.build_parser().parse_args(["--rom", "r", "--elf", "e"]).profile,
                         "protoss-p6")
        self.assertEqual(startup.build_parser().parse_args(
            ["--profile", "zerg-z6", "--rom", "r", "--elf", "e"]).profile, "zerg-z6")
        events = startup.startup_input_events(16)
        self.assertEqual(events, [(tick - 1, held)
                                  for tick, held in startup.replay.SIXTH_MENU_PREFIX])
        self.assertEqual(len(events), 12)
        self.assertEqual(events[-1], (11, 0))

    def test_rejects_short_event_queue_and_nonbounded_measurement_values(self):
        with self.assertRaisesRegex(ValueError, "capacity"):
            startup.startup_input_events(11)
        for value in (0, -1, startup.MAX_LIVE_FRAMES + 1, True):
            with self.assertRaises(ValueError):
                startup.validate_live_frames(value)
        for value in (99, 101, True):
            with self.assertRaises(ValueError):
                startup.validate_checkpoint(value)

    def test_checkpoint_targets_include_firmware_and_requested_boundaries(self):
        self.assertEqual(startup.startup_checkpoint_ticks(12, 1000, 1000), [1000, 1012])
        self.assertEqual(startup.startup_checkpoint_ticks(12, 250, 100),
                         [112, 212, 262])
        with self.assertRaisesRegex(ValueError, "32-bit"):
            startup.startup_checkpoint_ticks(startup.MAX_U32, 1, 100)

    def test_profile_contract_requires_real_playable_protoss_mission_25(self):
        contract = startup.verify_profile_contract(catalog(), symbols(), sizes())
        self.assertEqual(contract["mission"], 25)
        self.assertEqual(contract["scheduler_mode"], "windowed")
        for bad in (
            {"profile": "zerg", "enabled_mission_ids": [25],
             "records": [{"id": 25, "faction": 2, "playable": 1}]},
            {"profile": "protoss", "enabled_mission_ids": [],
             "records": [{"id": 25, "faction": 2, "playable": 1}]},
            {"profile": "protoss", "enabled_mission_ids": [25],
             "records": [{"id": 25, "faction": 2, "playable": 0}]},
        ):
            with self.assertRaises(ValueError):
                startup.verify_profile_contract(bad, symbols(), sizes())
        bad_symbols = symbols(); del bad_symbols["race_p6"]
        with self.assertRaisesRegex(RuntimeError, "race_p6"):
            startup.verify_profile_contract(catalog(), bad_symbols, sizes())

    def test_live_state_rejects_wrong_screen_terminal_or_bad_runtime(self):
        startup.validate_p6_live_state(state())
        for mutate in (
            lambda value: value.update(screen=3),
            lambda value: value.update(race_selected_mission=24),
            lambda value: value.update(won=1),
            lambda value: value["race_runtime"].update(fault=1),
            lambda value: value["race_runtime"]["model"].update(fired=0),
        ):
            broken = copy.deepcopy(state()); mutate(broken)
            with self.assertRaises(RuntimeError):
                startup.validate_p6_live_state(broken)

    def test_comparison_requires_startup_reports_and_compares_exact_state_and_capture(self):
        baseline = report()
        candidate = copy.deepcopy(baseline)
        candidate["rom_sha256"] = "3" * 64
        candidate["elf_sha256"] = "4" * 64
        result = startup.compare_startup_reports(baseline, candidate)
        self.assertTrue(result["exact_state_match"])
        self.assertTrue(result["exact_capture_match"])
        candidate["checkpoints"][0]["state"]["frame"] += 1
        result = startup.compare_startup_reports(baseline, candidate)
        self.assertFalse(result["exact_state_match"])
        self.assertTrue(result["exact_capture_match"])
        candidate["checkpoints"][0]["capture_sha256"] = "b" * 64
        result = startup.compare_startup_reports(baseline, candidate)
        self.assertFalse(result["exact_capture_match"])
        invalid = copy.deepcopy(baseline); invalid["qualification"] = "win"
        with self.assertRaisesRegex(ValueError, "startup-only"):
            startup.compare_startup_reports(invalid, baseline)

    def test_expected_hash_syntax_is_strict(self):
        self.assertEqual(startup.validate_expected_hash("A" * 64, "ROM"), "a" * 64)
        for value in ("x" * 64, "0" * 63, "0" * 65):
            with self.assertRaisesRegex(ValueError, "64 hexadecimal"):
                startup.validate_expected_hash(value, "ELF")

    def test_z6_profile_contract_requires_all_six_zerg_records_and_runtime_symbol(self):
        contract = startup.verify_profile_contract(z6_catalog(), z6_symbols(), sizes(), "zerg-z6")
        self.assertEqual(contract["mission"], 17)
        self.assertEqual(contract["profile"], "zerg-z6")
        self.assertEqual(contract["scheduler_mode"], "windowed")
        for bad in (
            {"profile": "protoss", "enabled_mission_ids": [17],
             "records": z6_catalog()["records"]},
            {"profile": "zerg", "enabled_mission_ids": [0, 1, 14, 15, 16, 17],
             "records": z6_catalog()["records"][:-1]},
            {"profile": "zerg", "enabled_mission_ids": [0, 1, 14, 15, 16],
             "records": z6_catalog()["records"]},
        ):
            with self.assertRaises(ValueError):
                startup.verify_profile_contract(bad, z6_symbols(), sizes(), "zerg-z6")
        bad_symbols = z6_symbols(); del bad_symbols["race_z6"]
        with self.assertRaisesRegex(RuntimeError, "race_z6"):
            startup.verify_profile_contract(z6_catalog(), bad_symbols, sizes(), "zerg-z6")

    def test_z6_startup_state_requires_live_unfinished_nine_trigger_snapshot(self):
        startup.validate_z6_live_state(z6_state())
        broken = copy.deepcopy(z6_state()); broken["race_selected_mission"] = 16
        with self.assertRaises(RuntimeError): startup.validate_z6_live_state(broken)
        broken = copy.deepcopy(z6_state()); broken["race_runtime"]["model"]["started"] = 1 << 9
        with self.assertRaises(RuntimeError): startup.validate_z6_live_state(broken)
        broken = copy.deepcopy(z6_state()); broken["race_runtime"]["action_ticks"][0] = 0
        with self.assertRaises(RuntimeError): startup.validate_z6_live_state(broken)

    def test_z6_research_timer_requires_candidate_producer_relation(self):
        for transport in ([0, 0, 0], [3000, 1, 0], [1, 160, 0], [0, 0, 1]):
            valid = z6_state(); valid["race_runtime"]["transport"] = transport
            startup.validate_z6_live_state(valid)
        for transport in ([3000, 0, 0], [3000, 161, 0], [0, 1, 0], [1, 1, 1]):
            broken = z6_state(); broken["race_runtime"]["transport"] = transport
            with self.assertRaisesRegex(RuntimeError, "producer/timer"):
                startup.validate_z6_live_state(broken)

    def test_z6_report_comparison_cannot_mix_profile_with_p6(self):
        result = startup.compare_startup_reports(z6_report(), z6_report())
        self.assertTrue(result["exact_state_match"])
        with self.assertRaisesRegex(ValueError, "different profiles"):
            startup.compare_startup_reports(report(), z6_report())


if __name__ == "__main__":
    unittest.main()
