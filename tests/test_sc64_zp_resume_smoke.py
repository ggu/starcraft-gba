import hashlib
import json
import sys
import tempfile
import unittest
from pathlib import Path

TOOLS = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS))

import sc64_zp_resume_smoke as resume


def state(screen=2):
    return {
        "screen": screen, "frame": 1000, "won": 0, "lost": 0,
        "race_mode": 0, "race_selected_mission": 15,
        "race_campaign": {"mission": 15, "elapsed_frames": 1000,
                           "objective_flags": 0, "phase": 1},
        "race_runtime": {"model": {"fired": 3, "outcome": 0,
                                    "action_cursor": [1, 2, 0]},
                         "action_cursor": [1, 0], "faults": 0},
        "race_minerals": [10, 20, 30], "race_gas": [2, 3, 4],
        "units": {"alive": 1, "entities": [{"slot": 1, "owner": 0,
                                                "type": 1, "raw_type": 51,
                                                "hp": 20, "shield": 0}]},
        "buildings": {"alive": 1, "entities": [{"slot": 0, "owner": 0,
                                                     "type": 1, "raw_type": 106,
                                                     "hp": 100}]},
    }


class ResumeAuthenticationTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        self.rom = self.root / "mission.gba"
        self.elf = self.root / "mission.elf"
        self.sram = self.root / "mission.sav"
        self.report = self.root / "profile.json"
        self.rom.write_bytes(b"ROM fixture")
        self.elf.write_bytes(b"ELF fixture")
        self.sram.write_bytes(bytes(range(256)) * 128)
        saved = state()
        digest = hashlib.sha256(self.sram.read_bytes()).hexdigest()
        payload = {
            "status": "prefix", "prefix": True, "profile": "zerg-z4", "mission": 15,
            "requested_ticks": 1000,
            "rom_sha256": hashlib.sha256(self.rom.read_bytes()).hexdigest(),
            "elf_sha256": hashlib.sha256(self.elf.read_bytes()).hexdigest(),
            "save_checkpoint": {"path": str(self.sram), "sha256": digest,
                                "saved_frame": saved["frame"], "state": saved},
        }
        self.report.write_text(json.dumps(payload))

    def tearDown(self):
        self.temp.cleanup()

    def test_authenticates_matching_build_and_exact_sram(self):
        auth = resume.validate_source_report(self.report, self.rom, self.elf)
        self.assertEqual(auth["profile"], "zerg-z4")
        self.assertEqual(auth["mission"], 15)
        self.assertEqual(auth["source_sram_sha256"],
                         hashlib.sha256(self.sram.read_bytes()).hexdigest())

    def test_rejects_crossbuild_hash_mismatch(self):
        with self.assertRaisesRegex(ValueError, "ROM SHA-256"):
            resume.validate_source_report(self.report, self.rom, self.elf,
                                          expected_rom_sha256="0" * 64)
        payload = json.loads(self.report.read_text()); payload["elf_sha256"] = "0" * 64
        self.report.write_text(json.dumps(payload))
        with self.assertRaisesRegex(ValueError, "ROM/ELF hashes"):
            resume.validate_source_report(self.report, self.rom, self.elf)

    def test_rejects_corrupt_or_wrong_size_sram(self):
        self.sram.write_bytes(b"bad")
        with self.assertRaisesRegex(ValueError, "exactly 32768"):
            resume.validate_source_report(self.report, self.rom, self.elf)
        self.sram.write_bytes(bytes(range(256)) * 128)
        data = bytearray(self.sram.read_bytes()); data[0] ^= 1; self.sram.write_bytes(data)
        with self.assertRaisesRegex(ValueError, "SHA-256"):
            resume.validate_source_report(self.report, self.rom, self.elf)

    def test_rejects_report_without_a_live_pause_snapshot_or_checkpoint(self):
        payload = json.loads(self.report.read_text()); payload["status"] = "won"
        self.report.write_text(json.dumps(payload))
        with self.assertRaisesRegex(ValueError, "bounded live-play prefix"):
            resume.validate_source_report(self.report, self.rom, self.elf)
        payload["status"] = "prefix"; payload["save_checkpoint"]["state"]["screen"] = 1
        self.report.write_text(json.dumps(payload))
        with self.assertRaisesRegex(ValueError, "PAUSE screen"):
            resume.validate_source_report(self.report, self.rom, self.elf)

    def test_restored_state_allows_only_root_pause_to_play_change(self):
        resume.compare_saved_state(state(2), state(1))
        changed = state(1); changed["race_campaign"]["elapsed_frames"] += 1
        with self.assertRaisesRegex(RuntimeError, "race_campaign.elapsed_frames"):
            resume.compare_saved_state(state(2), changed)
        changed = state(1); changed["units"]["entities"][0]["hp"] -= 1
        with self.assertRaisesRegex(RuntimeError, "units.entities"):
            resume.compare_saved_state(state(2), changed)
        changed = state(1); changed["race_runtime"]["model"]["action_cursor"][0] = 0
        with self.assertRaisesRegex(RuntimeError, "race_runtime.model.action_cursor"):
            resume.compare_saved_state(state(2), changed)

    def test_neutral_handoff_and_measured_tick_continuity_are_explicit(self):
        state_after = {"frame": 1061, "screen": 1}
        result = resume.verify_handoff_and_resume(1000, 1001, 60, 60, state_after)
        self.assertEqual(result["handoff_live_frames"], 1)
        self.assertTrue(result["frame_invariant_verified"])
        with self.assertRaisesRegex(RuntimeError, "exactly one neutral"):
            resume.verify_handoff_and_resume(1000, 1002, 60, 60, state_after)
        with self.assertRaisesRegex(RuntimeError, "frame progression"):
            resume.verify_handoff_and_resume(1000, 1001, 60, 59,
                                             {"frame": 1060, "screen": 1})


if __name__ == "__main__":
    unittest.main()
