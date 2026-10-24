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


def z6_runtime():
    return {
        "model": {"started": 0, "fired": 0, "outcome": 0,
                  "action_cursor": [0] * 9},
        "action_ticks": [0] * 9, "switch_bits": [0] * 8, "fault": 0,
        "deaths_by_source": [0] * 12, "moved_location": [0] * 8,
        "moved_locations_mask": 0, "ai_plan_ticks": 0, "ai_plan_phase": 0,
        "human_drone_morphs": 0, "cloak": [0, 0], "storm": [0] * 32,
        "transport": [0, 0, 0], "persistent_size": 176,
        "persistent_bytes": [0] * 176,
    }


def z6_state(screen=2):
    result = state(screen)
    result["race_selected_mission"] = 17
    result["race_campaign"]["mission"] = 17
    result["race_runtime"] = z6_runtime()
    result["units"]["entities"][0].update({
        "owner": 0, "raw_owner": 3, "raw_type": 51, "hp": 200,
        "order": 0, "target": -1,
    })
    return result


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

    def test_authenticates_fifth_and_sixth_entry_pause_snapshots(self):
        base = json.loads(self.report.read_text())
        for profile_name, mission in (("zerg-z5", 16), ("protoss-p5", 24),
                                      ("protoss-p6", 25)):
            with self.subTest(profile=profile_name):
                payload = copy_report = json.loads(json.dumps(base))
                payload["profile"] = profile_name
                payload["mission"] = mission
                saved = payload["save_checkpoint"]["state"]
                saved["race_selected_mission"] = mission
                saved["race_campaign"]["mission"] = mission
                saved["race_runtime"] = {
                    "model": {"outcome": 0,
                              "action_cursor": list(range(40 if mission == 16 else 11)),
                              "started_lo": 0x89ABCDEF, "started_hi": 0x01234567,
                              "fired": 0x0123456789ABCDEF},
                    "action_cursor": list(range(40 if mission == 16 else 11)),
                    "action_ticks": list(range(40 if mission == 16 else 11)),
                    "switch_bits": [0x11111111 * i for i in range(8)],
                    "faults": 0, "persistent_bytes": list(range(181 if mission == 16 else 123)),
                }
                if profile_name == "protoss-p6":
                    saved["race_runtime"] = {
                        "model": {"outcome": 0, "started": 0, "fired": 0,
                                  "action_cursor": [0] * 26},
                        "action_ticks": [0] * 26, "switch_bits": 0, "fault": 0,
                        "deaths_by_source": [0] * 12,
                        "doodad_enabled": [0, 1, 0, 0, 0x1F],
                        "pause_acknowledged": 1, "reserved": [0, 0],
                        "storm": [0]*32, "persistent_bytes": [0] * 180,
                    }
                saved["buildings"]["entities"][0]["queue"] = [1, 7, 3, 9, 5]
                self.report.write_text(json.dumps(payload))
                auth = resume.validate_source_report(self.report, self.rom, self.elf)
                self.assertEqual(auth["profile"], profile_name)
                self.assertEqual(auth["mission"], mission)
                self.assertEqual(auth["source_state"]["race_runtime"]["persistent_bytes"][-1],
                                 (180 if mission == 16 else 0 if mission == 25 else 122))
                self.assertEqual(auth["source_state"]["buildings"]["entities"][0]["queue"],
                                 [1, 7, 3, 9, 5])
        self.report.write_text(json.dumps(base))

    def test_p6_resume_seed_rejects_incomplete_persistent_observations(self):
        payload = json.loads(self.report.read_text())
        payload["profile"], payload["mission"] = "protoss-p6", 25
        saved = payload["save_checkpoint"]["state"]
        saved["race_selected_mission"] = 25
        saved["race_campaign"]["mission"] = 25
        saved["race_runtime"] = {
            "model": {"outcome": 0, "started": 0, "action_cursor": [0] * 26},
            "action_ticks": [0] * 26, "switch_bits": 0, "fault": 0,
            "deaths_by_source": [0] * 12, "doodad_enabled": [0] * 5,
            "pause_acknowledged": 0, "reserved": [0, 0],
            "storm": [0]*32, "persistent_bytes": [0] * 180,
        }
        self.report.write_text(json.dumps(payload))
        resume.validate_source_report(self.report, self.rom, self.elf)
        for field, changed, error in (
                ("action_ticks", [0] * 25, "action timers"),
                ("deaths_by_source", [0] * 11, "death counters"),
                ("doodad_enabled", [0, 0, 0, 0, 0x20], "doodad enabled"),
                ("doodad_enabled", [1, 0, 0, 0, 0], "doodad enabled"),
                ("doodad_enabled", [0, 0xF0, 0, 0, 0], "doodad enabled"),
                ("doodad_enabled", [0, 0, 0x9B, 0, 0], "doodad enabled"),
                ("persistent_bytes", [0] * 179, "180 persistent bytes"),
                ("storm", [0] * 31, "Storm state"),
                ("storm", [1]+[0]*31, "Storm state"),
                ("reserved", [0, 0, 1], "reserved bytes")):
            broken = json.loads(json.dumps(payload))
            broken["save_checkpoint"]["state"]["race_runtime"][field] = changed
            self.report.write_text(json.dumps(broken))
            with self.subTest(field=field), self.assertRaisesRegex(ValueError, error):
                resume.validate_source_report(self.report, self.rom, self.elf)
        for name, mutate, error in (
                ("cursor", lambda r: r["model"]["action_cursor"].__setitem__(4, 1),
                 "cursor/timer state"),
                ("switch", lambda r: r.__setitem__("switch_bits", 0x40),
                 "switch bits")):
            broken = json.loads(json.dumps(payload))
            mutate(broken["save_checkpoint"]["state"]["race_runtime"])
            self.report.write_text(json.dumps(broken))
            with self.subTest(name=name), self.assertRaisesRegex(ValueError, error):
                resume.validate_source_report(self.report, self.rom, self.elf)

    def _write_z6_report(self, runtime=None, *, hero=True):
        payload = json.loads(self.report.read_text())
        saved = z6_state()
        if runtime is not None:
            saved["race_runtime"] = runtime
        if not hero:
            saved["units"]["entities"] = []
        payload["profile"], payload["mission"] = "zerg-z6", 17
        payload["save_checkpoint"]["state"] = saved
        payload["save_checkpoint"]["saved_frame"] = saved["frame"]
        self.report.write_text(json.dumps(payload))

    def test_z6_resume_seed_authenticates_full_live_prefix(self):
        self._write_z6_report()
        auth = resume.validate_source_report(self.report, self.rom, self.elf)
        self.assertEqual(auth["profile"], "zerg-z6")
        self.assertEqual(auth["mission"], 17)
        self.assertEqual(len(auth["source_state"]["race_runtime"]["persistent_bytes"]), 176)

    def test_z6_live_entity_observer_shape_does_not_require_alive_field(self):
        self._write_z6_report()
        state_data = json.loads(self.report.read_text())["save_checkpoint"]["state"]
        self.assertNotIn("alive", state_data["units"]["entities"][0])
        resume.validate_z6_prefix_state(state_data)
        state_data["units"]["entities"][0]["hp"] = 0
        with self.assertRaisesRegex(ValueError, "source Kerrigan"):
            resume.validate_z6_prefix_state(state_data)

    def test_z6_resume_seed_rejects_malformed_persistent_relations(self):
        cases = (
            (lambda r: r["model"].update(started=1), "model/cursor/timer"),
            (lambda r: r["model"]["action_cursor"].__setitem__(0, 2), "cursor/timer"),
            (lambda r: r["action_ticks"].__setitem__(0, 1), "cursor/timer"),
            (lambda r: r["switch_bits"].__setitem__(0, 4), "switches or runtime fault"),
            (lambda r: r["switch_bits"].__setitem__(1, 1), "switches or runtime fault"),
            (lambda r: r["deaths_by_source"].__setitem__(3, 65536), "death counters"),
            (lambda r: r["moved_location"].__setitem__(2, 1), "unmoved MRGN"),
            (lambda r: r.update(moved_locations_mask=4), "moved MRGN"),
            (lambda r: r.update(ai_plan_ticks=1), "AI cadence before"),
            (lambda r: r.update(ai_plan_phase=4), "AI cadence"),
            (lambda r: r.update(cloak=[1, 1]), "cloak state"),
            (lambda r: r.update(storm=[1] + [0] * 31), "Storm state"),
            (lambda r: r.update(transport=[3001, 1, 0]), "transport research timer"),
            (lambda r: r.update(transport=[1, 0, 0]), "transport research timer"),
            (lambda r: r.update(transport=[1, 1, 1]), "transport research timer"),
            (lambda r: r.update(transport=[1, 161, 0]), "transport research timer"),
            (lambda r: r.update(human_drone_morphs=5), "176 persistent bytes"),
            (lambda r: r.update(persistent_size=175), "176 persistent bytes"),
            (lambda r: r.update(persistent_size=181), "176 persistent bytes"),
            (lambda r: r.update(persistent_bytes=[0] * 175), "176 persistent bytes"),
        )
        for mutate, error in cases:
            runtime = z6_runtime()
            mutate(runtime)
            self._write_z6_report(runtime)
            with self.subTest(runtime=runtime), self.assertRaisesRegex(ValueError, error):
                resume.validate_source_report(self.report, self.rom, self.elf)
        self._write_z6_report(hero=False)
        with self.assertRaisesRegex(ValueError, "source Kerrigan"):
            resume.validate_source_report(self.report, self.rom, self.elf)
        for field, value in (("alive", 0), ("raw_owner", 2), ("raw_type", 79)):
            saved = z6_state()
            saved["units"]["entities"][0][field] = value
            self._write_z6_report(saved["race_runtime"])
            payload = json.loads(self.report.read_text())
            payload["save_checkpoint"]["state"]["units"] = saved["units"]
            self.report.write_text(json.dumps(payload))
            with self.subTest(hero_field=field), self.assertRaisesRegex(ValueError, "source Kerrigan"):
                resume.validate_source_report(self.report, self.rom, self.elf)

    def test_z6_save_resume_comparison_includes_transport_and_all_payload_bytes(self):
        saved = z6_state()
        saved["race_runtime"].update({
            "model": {"started": 0x100, "fired": 0x100, "outcome": 0,
                      "action_cursor": [0] * 8 + [1]},
            "action_ticks": [0] * 8 + [90],
            "ai_plan_ticks": 240, "ai_plan_phase": 2,
            "moved_location": [1, 2, 3, 4, 0, 0, 0, 0],
            "moved_locations_mask": 1, "transport": [2999, 160, 0],
        })
        restored = json.loads(json.dumps(saved))
        restored["screen"] = 1
        resume.compare_saved_state(saved, restored)
        for key, mutate in (
                ("transport", lambda v: v.__setitem__(0, 2998)),
                ("persistent_bytes", lambda v: v.__setitem__(-1, 1))):
            changed = json.loads(json.dumps(restored))
            mutate(changed["race_runtime"][key])
            with self.subTest(field=key), self.assertRaisesRegex(RuntimeError, "race_runtime"):
                resume.compare_saved_state(saved, changed)
        changed = json.loads(json.dumps(restored))
        changed["units"]["entities"][0]["order"] = 7
        with self.assertRaisesRegex(RuntimeError, "units.entities"):
            resume.compare_saved_state(saved, changed)

    def test_restored_persistent_snapshot_compares_every_nested_byte(self):
        saved = state(2)
        saved["race_selected_mission"] = 24
        saved["race_campaign"]["mission"] = 24
        saved["race_runtime"] = {
            "model": {"outcome": 0, "action_cursor": [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11],
                      "started": 0x07F3, "fired": 0x07F3},
            "action_cursor": [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11],
            "action_ticks": list(range(11)),
            "switch_bits": [0xA0000000 + i for i in range(8)],
            "deaths_by_source": list(range(12)),
            "pressure_dispatched_mask": 5, "reserved": [0, 0, 0], "faults": 0,
            "persistent_bytes": list(range(123)),
        }
        saved["buildings"]["entities"][0]["queue"] = [9, 8, 7, 6, 5]
        restored = json.loads(json.dumps(saved)); restored["screen"] = 1
        resume.compare_saved_state(saved, restored)
        for path, mutate in (
                ("race_runtime.model.action_cursor", lambda x: x.__setitem__(0, 0)),
                ("race_runtime.switch_bits", lambda x: x.__setitem__(0, 0)),
                ("race_runtime.persistent_bytes", lambda x: x.__setitem__(-1, 0)),
                ("buildings.entities.0.queue", lambda x: x.__setitem__(-1, 0))):
            changed = json.loads(json.dumps(restored))
            value = changed
            for key in path.split("."):
                value = value[int(key)] if isinstance(value, list) else value[key]
            mutate(value)
            with self.subTest(path=path), self.assertRaisesRegex(RuntimeError, path.split(".")[0]):
                resume.compare_saved_state(saved, changed)


if __name__ == "__main__":
    unittest.main()
