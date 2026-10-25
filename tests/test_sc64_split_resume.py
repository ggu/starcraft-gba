"""Asset-free tests for authenticated fresh-process campaign resume checks."""
from __future__ import annotations

import hashlib
import json
from pathlib import Path
import tempfile
import unittest

from tools import sc64_split_profile as resume


class SplitResumeQualificationTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.save = self.root / "midmission.sav"
        self.save.write_bytes(b"s" * 32768)
        self.rom_hash = "a" * 64
        self.elf_hash = "b" * 64
        self.state = {
            "screen": 2, "frame": 12345, "won": 0, "lost": 0,
            "race_mode": 0, "race_human": 1, "race_selected_mission": 14,
            "race_completed": 1 << 14,
            "minerals": 320, "gas": 180,
            "units": {"alive": 2, "entities": [{"slot": 1, "hp": 80}]},
            "buildings": {"alive": 1, "entities": [{"slot": 3, "type": 14, "hp": 750}]},
            "campaign": {"mission": 14, "elapsed_frames": 12345,
                          "phase": 1, "objective_flags": 2},
            "race_z3": {"outcome": 0, "sequence": 3, "fired": 9,
                        "switch7": 0, "transmission": 0, "action_ticks": 0},
        }
        self.report = self.root / "profile.json"
        self.write_report()

    def write_report(self, **changes):
        checkpoint = {"state": self.state,
                      "sha256": hashlib.sha256(self.save.read_bytes()).hexdigest()}
        checkpoint.update(changes)
        payload = {"profile": "zerg-z3", "mission": 14,
                   "rom_sha256": self.rom_hash, "elf_sha256": self.elf_hash,
                   "checkpoint_save": checkpoint}
        self.report.write_text(json.dumps(payload), encoding="utf-8")

    def test_report_authenticates_matching_build_and_sram(self):
        self.assertEqual(json.loads(self.report.read_text(encoding="utf-8"))["profile"],
                         "zerg-z3")
        self.assertEqual(
            resume.saved_state_from_report(
                self.report, self.save, profile="zerg", mission=14,
                rom_sha256=self.rom_hash, elf_sha256=self.elf_hash), self.state)
        for kwargs, message in (
            ({"profile": "protoss"}, "profile/mission"),
            ({"mission": 22}, "profile/mission"),
            ({"rom_sha256": "c" * 64}, "ROM/ELF hashes"),
            ({"elf_sha256": "d" * 64}, "ROM/ELF hashes"),
        ):
            args = {"profile": "zerg", "mission": 14,
                    "rom_sha256": self.rom_hash, "elf_sha256": self.elf_hash}
            args.update(kwargs)
            with self.subTest(kwargs=kwargs), self.assertRaisesRegex(ValueError, message):
                resume.saved_state_from_report(self.report, self.save, **args)

        self.save.write_bytes(b"x" * 32768)
        with self.assertRaisesRegex(ValueError, "does not authenticate"):
            resume.saved_state_from_report(
                self.report, self.save, profile="zerg", mission=14,
                rom_sha256=self.rom_hash, elf_sha256=self.elf_hash)

    def test_restored_state_must_match_complete_world_and_model(self):
        observed = dict(self.state)
        observed.update(screen=1, race_campaign=self.state["campaign"])
        del observed["campaign"]
        resume.verify_restored_checkpoint(self.state, observed, mission=14)

        for key in ("frame", "race_mode", "race_selected_mission", "race_human",
                    "minerals", "gas", "units", "buildings"):
            changed = dict(observed)
            changed[key] = None
            with self.subTest(key=key), self.assertRaisesRegex(RuntimeError, key):
                resume.verify_restored_checkpoint(self.state, changed, mission=14)

        changed = dict(observed, race_z3={"outcome": 1})
        with self.assertRaisesRegex(RuntimeError, "race_z3"):
            resume.verify_restored_checkpoint(self.state, changed, mission=14)
        changed = dict(observed)
        del changed["race_completed"]
        with self.assertRaisesRegex(RuntimeError, "race_completed"):
            resume.verify_restored_checkpoint(self.state, changed, mission=14)
        changed = dict(observed, screen=2)
        with self.assertRaisesRegex(RuntimeError, "live campaign"):
            resume.verify_restored_checkpoint(self.state, changed, mission=14)

    def test_report_verification_requires_mission_model_and_campaign(self):
        for missing in ("race_z3", "campaign"):
            expected = dict(self.state)
            del expected[missing]
            with self.subTest(missing=missing), self.assertRaisesRegex(RuntimeError, "missing"):
                resume.verify_restored_checkpoint(expected, dict(self.state, screen=1), mission=14)

        p3_campaign = dict(self.state["campaign"], mission=22)
        p3_state = dict(self.state, race_selected_mission=22, race_p3={"outcome": 0},
                        race_campaign=p3_campaign)
        del p3_state["race_z3"]
        del p3_state["campaign"]
        resume.verify_restored_checkpoint(p3_state, dict(p3_state, screen=1), mission=22)
        del p3_state["race_p3"]
        with self.assertRaisesRegex(RuntimeError, "missing"):
            resume.verify_restored_checkpoint(p3_state, dict(p3_state, screen=1), mission=22)

    def test_legacy_load_without_report_remains_supported(self):
        resume.validate_load_save_options(self.save, None, 6)
        resume.validate_load_save_options(self.save, None, 14)
        with self.assertRaisesRegex(ValueError, "requires --mission"):
            resume.validate_load_save_options(self.save, None, None)
        with self.assertRaisesRegex(ValueError, "requires --load-save"):
            resume.validate_load_save_options(None, self.report, 14)
        with self.assertRaisesRegex(ValueError, "supports Z3 mission 14"):
            resume.validate_load_save_options(self.save, self.report, 6)

    def test_resume_frame_invariant_and_early_end_reporting(self):
        live = resume.resumed_frame_continuity(
            1200, 1201, 10000, 10000, 10000, {"screen": 1, "frame": 11201})
        self.assertTrue(live["frame_invariant_verified"])
        self.assertEqual(live["handoff_live_frames"], 1)
        self.assertEqual(live["completed_scheduler_live_ticks"], 10000)
        self.assertEqual(live["observed_simulation_frame_advance_since_load"], 10001)
        self.assertEqual(live["observed_simulation_frame_advance_after_tick_zero"], 10000)
        early = resume.resumed_frame_continuity(
            1200, 1201, 10000, 4000, 3000, {"screen": 3, "frame": 5201})
        self.assertFalse(early["frame_invariant_verified"])
        self.assertTrue(early["full_live_interval_not_verified"])
        self.assertEqual(early["completed_scheduler_ticks"], 4000)
        self.assertEqual(early["completed_scheduler_live_ticks"], 3000)
        self.assertEqual(early["observed_simulation_frame_advance_since_load"], 4001)
        self.assertEqual(early["observed_simulation_frame_advance_after_tick_zero"], 4000)
        endpoint_result = resume.resumed_frame_continuity(
            1200, 1201, 10000, 10000, 9000, {"screen": 3, "frame": 11201})
        self.assertFalse(endpoint_result["frame_invariant_verified"])
        self.assertTrue(endpoint_result["full_live_interval_not_verified"])
        self.assertEqual(endpoint_result["observed_simulation_frame_advance_after_tick_zero"], 10000)
        with self.assertRaisesRegex(RuntimeError, "frame does not match"):
            resume.resumed_frame_continuity(
                1200, 1201, 10000, 10000, 10000, {"screen": 1, "frame": 11200})
        for tick_zero in (1200, 1202):
            with self.subTest(tick_zero=tick_zero), self.assertRaisesRegex(RuntimeError, "exactly one"):
                resume.resumed_frame_continuity(
                    1200, tick_zero, 10000, 10000, 10000,
                    {"screen": 1, "frame": tick_zero + 10000})

    def test_observer_reads_building_hp_for_snapshot_schema_parity(self):
        class FakeRsp:
            def read(self, address, length):
                self.asserted = (address, length)
                return (750).to_bytes(4, "little", signed=True)

        buildings = {"entities": [{"slot": 3, "type": 14}]}
        observed = resume.add_building_hp(
            FakeRsp(), {"b": 0x1000},
            {"building_offsets": {"hp": 8}, "building_stride_bytes": 32},
            buildings)
        self.assertEqual(observed["entities"][0]["hp"], 750)


if __name__ == "__main__":
    unittest.main()
