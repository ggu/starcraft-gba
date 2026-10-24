import hashlib
import json
import tempfile
import unittest
from pathlib import Path

from tools import sc64_p3_controller_replay as replay
from tools import sc64_p3_resume_smoke as resume
from tools import sc64_split_profile as profile


def source_state(frame=123456):
    return {
        "screen": 2, "frame": frame,
        "race_mode": 0, "race_selected_mission": 22, "won": 0, "lost": 0,
        "race_human": 0, "race_completed": 0,
        "race_campaign": {"mission": 22, "elapsed_frames": frame,
                           "phase": 1, "objective_flags": 0},
        "race_p3": {"outcome": 0, "sequence": 0, "fired": 1,
                    "transmission": 0, "reserved": 0, "action_ticks": 0},
        "race_p3_action_ticks": 0,
        "units": {"entities": []}, "buildings": {"entities": []},
        "raw_owner7_units": 14, "raw_owner0_units": 43, "raw_owner6_units": 39,
        "raw_owner7_buildings": 4, "raw_owner0_buildings": 22, "raw_owner6_buildings": 16,
        "geyser_count": 3, "selected_units": 0, "selected_building": -1,
    }


class P3ExpansionResume(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        root = Path(self.temp.name)
        self.rom, self.elf, self.sram = root / "game.gba", root / "game.elf", root / "state.sav"
        self.rom.write_bytes(b"rom contents")
        self.elf.write_bytes(b"elf contents")
        self.sram.write_bytes(bytes(32 * 1024))
        self.frame = 123456
        self.report = {
            "profile": "protoss-p3", "route": "expansion", "policy": replay.EXPANSION_POLICY,
            "status": "prefix", "prefix": True, "verified_win": False,
            "mission": 22, "requested_ticks": 60000, "observed_ticks": 60000,
            "input_transitions": 5932,
            "rom_sha256": resume.sha256_file(self.rom), "elf_sha256": resume.sha256_file(self.elf),
            "final": {"screen": 1, "won": 0, "lost": 0, "race_mode": 0,
                      "race_selected_mission": 22,
                      "race_p3": {"outcome": 0, "sequence": 1}},
            "save": {"observer_tick": 60000, "observer_frame": self.frame,
                     "saved_frame": self.frame, "input": "START pause, B save",
                     "state": source_state(self.frame), "path": str(self.sram),
                     "sha256": resume.sha256_file(self.sram)},
        }
        self.report_path = root / "profile.json"
        self.write_report()

    def tearDown(self):
        self.temp.cleanup()

    def write_report(self):
        self.report_path.write_text(json.dumps(self.report), encoding="utf-8")

    def test_valid_authenticated_prefix(self):
        auth = resume.validate_source_report(self.report_path, self.rom, self.elf)
        self.assertEqual(auth["source_state"]["frame"], self.frame)
        self.assertEqual(auth["source_sram_sha256"], self.report["save"]["sha256"])

    def test_rejects_build_hash_or_cross_build_mismatch(self):
        with self.assertRaisesRegex(ValueError, "ROM SHA-256"):
            resume.validate_source_report(self.report_path, self.rom, self.elf, "0" * 64)
        other = Path(self.temp.name) / "other.gba"
        other.write_bytes(b"different ROM")
        with self.assertRaisesRegex(ValueError, "ROM/ELF hashes"):
            resume.validate_source_report(self.report_path, other, self.elf)

    def test_rejects_bad_sram_size_or_digest(self):
        self.sram.write_bytes(b"short")
        with self.assertRaisesRegex(ValueError, "exactly 32768"):
            resume.validate_source_report(self.report_path, self.rom, self.elf)
        self.sram.write_bytes(bytes(32768))
        self.report["save"]["sha256"] = "0" * 64
        self.write_report()
        with self.assertRaisesRegex(ValueError, "does not match"):
            resume.validate_source_report(self.report_path, self.rom, self.elf)

    def test_rejects_wrong_route_terminal_or_missing_snapshot(self):
        for key, value, message in (("route", "garrison", "expansion route"),
                                    ("status", "won", "live-play prefix")):
            with self.subTest(key=key):
                self.report[key] = value
                self.write_report()
                with self.assertRaisesRegex(ValueError, message):
                    resume.validate_source_report(self.report_path, self.rom, self.elf)
                self.report[key] = "expansion" if key == "route" else "prefix"
        del self.report["save"]["state"]["buildings"]
        self.write_report()
        with self.assertRaisesRegex(ValueError, "complete unit/building"):
            resume.validate_source_report(self.report_path, self.rom, self.elf)

    def test_rejects_checkpoint_frame_model_and_observer_mismatch(self):
        self.report["save"]["state"]["frame"] += 1
        self.write_report()
        with self.assertRaisesRegex(ValueError, "timing"):
            resume.validate_source_report(self.report_path, self.rom, self.elf)
        self.report["save"]["state"] = source_state(self.frame)
        self.report["save"]["state"]["race_p3"]["outcome"] = 1
        self.write_report()
        with self.assertRaisesRegex(ValueError, "ongoing P3"):
            resume.validate_source_report(self.report_path, self.rom, self.elf)

    def test_restoration_compares_everything_except_pause_to_play(self):
        expected, observed = source_state(self.frame), source_state(self.frame)
        observed["screen"] = 1
        exact = resume.compare_saved_state(expected, observed)
        self.assertTrue(exact["persisted_observations_exact"])
        self.assertTrue(exact["restored_snapshot_exact"])
        expected["selected_building"] = 5
        reset = resume.compare_saved_state(expected, observed)
        self.assertEqual(reset["ui_resets"]["selected_building"], {"saved": 5, "loaded": -1})
        self.assertTrue(reset["persisted_observations_exact"])
        self.assertFalse(reset["restored_snapshot_exact"])

        observed["selected_building"] = 7
        with self.assertRaisesRegex(RuntimeError, "selected_building"):
            resume.compare_saved_state(expected, observed)
        observed["selected_building"] = -1
        observed["selected_units"] = 1
        with self.assertRaisesRegex(RuntimeError, "selected_units"):
            resume.compare_saved_state(expected, observed)
        observed["selected_units"] = 0
        observed["race_p3"]["fired"] = 3
        with self.assertRaisesRegex(RuntimeError, "race_p3.fired"):
            resume.compare_saved_state(expected, observed)
        observed["race_p3"]["fired"] = 1
        observed["units"]["entities"].append({"raw_type": 65})
        with self.assertRaisesRegex(RuntimeError, "entities length"):
            resume.compare_saved_state(expected, observed)

    def test_handoff_and_60_live_frames_are_strict(self):
        final = {"screen": 1, "frame": (self.frame + 61) & 0xFFFFFFFF}
        result = resume.verify_handoff_and_resume(self.frame, self.frame + 1, 60, final)
        self.assertEqual(result["handoff_live_frames"], 1)
        self.assertTrue(result["frame_invariant_verified"])
        with self.assertRaisesRegex(RuntimeError, "exactly one"):
            resume.verify_handoff_and_resume(self.frame, self.frame + 2, 60, final)
        final["frame"] += 1
        with self.assertRaisesRegex(RuntimeError, "frame"):
            resume.verify_handoff_and_resume(self.frame, self.frame + 1, 60, final)

    def test_cli_uses_shuttle_build_and_make_trace_defaults(self):
        args = resume.build_parser().parse_args(["--report", "report.json"])
        self.assertEqual(args.rom.name, "StarCraft-Protoss-P5-Shuttle-GBA.gba")
        self.assertEqual(args.elf.name, "starcraft-protoss-p5-shuttle.elf")
        self.assertEqual(replay.build_parser().parse_args(["--route", "expansion"]).trace,
                         None)
        self.assertEqual(replay.DEFAULT_EXPANSION_TRACE.name, "p3-expansion-controller.json")


if __name__ == "__main__":
    unittest.main()
