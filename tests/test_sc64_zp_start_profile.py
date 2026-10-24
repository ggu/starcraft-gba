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


class StartupProfileTests(unittest.TestCase):
    def test_default_scope_and_menu_prefix_are_exact(self):
        self.assertEqual(startup.DEFAULT_LIVE_FRAMES, 1000)
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


if __name__ == "__main__":
    unittest.main()
