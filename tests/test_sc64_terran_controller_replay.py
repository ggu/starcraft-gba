"""Asset-free source-trace validation for T6 and late Terran replay."""
from __future__ import annotations
import importlib.util
import json
import contextlib
import io
from pathlib import Path
import tempfile
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "sc64_terran_controller_replay", ROOT / "tools/sc64_terran_controller_replay.py")
assert SPEC and SPEC.loader
replay = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(replay)


def sample(mission=9, final="limit", stop=1200):
    prefix, launch = replay.menu_prefix(mission)
    start = {"kind": "start", "host_tick": 0, "frame": 0, "screen": 1,
             "mission": mission, "won": 0, "lost": 0, "faults": 0}
    rows = [start, {"kind": "input", "host_tick": 1, "held": 0, "pressed": 0}]
    progress = min(500, max(1, stop - 1))
    rows.append({"kind": "progress", "host_tick": progress, "frame": progress, "screen": 1,
                 "mission": mission, "won": 0, "lost": 0, "faults": 0})
    if final == "limit":
        rows.append({"kind": "limit", "host_tick": stop, "frame": stop,
                     "screen": 1, "mission": mission, "won": 0, "lost": 0, "faults": 0})
    else:
        rows.append({"kind": final, "host_tick": stop, "frame": stop,
                     "screen": 3, "mission": mission, "won": int(final == "won"),
                     "lost": int(final == "lost"), "faults": 0})
    return {"trace_schema": 1, "entry": "t6-probe-entry" if mission == 9 else f"t{mission-3}-probe-entry",
            "policy": "bounded-controller", "mission": mission, "tick_limit": stop,
            "events": rows}


class ReplayPlanTests(unittest.TestCase):
    def test_t6_goal_counts_match_source_owner_region_and_transport_filter(self):
        units = [
            {"owner": 0, "raw_type": 19, "order": 0, "x": 10, "y": 10},
            {"owner": 0, "raw_type": 11, "order": 8, "x": 10, "y": 10},
            {"owner": 1, "raw_type": 11, "order": 0, "x": 10, "y": 10},
            {"owner": 0, "raw_type": 11, "order": 0, "x": 49, "y": 10},
        ]
        # Source bounds are half-scale map coordinates; left/top are inclusive
        # and right/bottom are exclusive, matching the controller's predicate.
        bounds = (20, 20, 80, 80)
        self.assertEqual(replay._source_location_count(units, 19, bounds), 1)
        self.assertEqual(replay._source_location_count(units, 11, bounds), 0)

    def test_tracked_building_slots_are_identity_checked_only_when_present(self):
        trace = {"host_tick": 12, "starport_slot": -1, "tower_slot": -1}
        replay._compare_trace_row(trace, {"building_source_slots": {}})
        trace["starport_slot"] = 7
        with self.assertRaisesRegex(RuntimeError, "starport_slot identity"):
            replay._compare_trace_row(trace, {"building_source_slots": {114: [6]}})
        replay._compare_trace_row(trace, {"building_source_slots": {114: [7, 9]}})

    def test_t6_normal_menu_offset_and_page_bounds(self):
        plan = replay.validate_trace(sample(9, final="won", stop=1200), 256)
        self.assertEqual(plan["mission"], 9)
        self.assertEqual(plan["launch_tick"], 11)
        self.assertEqual(plan["start"]["host_tick"], 11)
        self.assertEqual(plan["final"]["host_tick"], 1211)
        pages = replay.make_pages(plan, plan["stop_tick"], 256)
        self.assertTrue(all(0 < p["end_tick"] - p["start_tick"] <= 1000 for p in pages))
        self.assertLessEqual(max(len(p["events"]) for p in pages), 256)

    def test_late_menu_route_by_chapter_and_row(self):
        expected = {10: 11, 11: 13, 12: 13, 13: 15}
        for mission, launch in expected.items():
            trace = sample(mission)
            plan = replay.validate_trace(trace, 256)
            self.assertEqual(plan["launch_tick"], launch)
            self.assertEqual(plan["start"]["host_tick"], launch)
            self.assertEqual(plan["inputs"][:len(plan["prefix"])], plan["prefix"])

    def test_prefix_budget_and_exact_nonmultiple_stop(self):
        plan = replay.validate_trace(sample(10, stop=1234), 256)
        pages = replay.make_pages(plan, 1000, 256)
        self.assertEqual(pages[-1]["end_tick"], 1000)
        self.assertTrue(all(p["end_tick"] - p["start_tick"] <= 1000 for p in pages))
        with self.assertRaisesRegex(ValueError, "capacity"):
            replay.make_pages(plan, 1000, 2)

    def test_wrong_entry_mission_or_menu_prefix_rejected(self):
        for field, value in (("entry", "t6-probe-entry"), ("mission", 8)):
            data = sample(10)
            data[field] = value
            with self.subTest(field=field), self.assertRaises(ValueError):
                replay.validate_trace(data, 256)
        data = sample(9)
        data["events"][-1]["host_tick"] = data["tick_limit"] + 1
        with self.assertRaises(ValueError):
            replay.validate_trace(data, 256)

    def test_limit_requires_exact_budget_and_play_screen(self):
        data = sample(9, stop=100)
        data["events"][-1]["host_tick"] = 99
        with self.assertRaisesRegex(ValueError, "budget"):
            replay.validate_trace(data, 256)

    def test_terminal_must_be_consistent(self):
        data = sample(10, final="won", stop=80)
        plan = replay.validate_trace(data, 256)
        self.assertEqual(plan["final"]["kind"], "won")
        data["events"][-1]["lost"] = 1
        with self.assertRaises(ValueError):
            replay.validate_trace(data, 256)

    def test_cli_runs_replay_by_default_and_plan_only_is_explicit(self):
        with tempfile.TemporaryDirectory() as directory:
            trace_path = Path(directory) / "trace.json"
            trace_path.write_text(json.dumps(sample(9)))
            args = ["--trace", str(trace_path), "--output", str(Path(directory) / "out"),
                    "--rom", str(Path(directory) / "rom.gba"),
                    "--elf", str(Path(directory) / "rom.elf")]
            with mock.patch.object(replay, "run", return_value={"status": "ran"}) as run:
                with contextlib.redirect_stdout(io.StringIO()):
                    self.assertEqual(replay.main(args), 0)
                run.assert_called_once()
            with mock.patch.object(replay, "run") as run:
                with contextlib.redirect_stdout(io.StringIO()):
                    self.assertEqual(replay.main(args + ["--validate-only"]), 0)
                run.assert_not_called()

if __name__ == "__main__":
    unittest.main()
