#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Asset-free tests for tools/sc64_campaign_check.py (no controllers are built or run)."""

from __future__ import annotations

import re
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

import sc64_campaign_check as cc  # noqa: E402
import sc64_rom_check as romcheck  # noqa: E402

# One real output line per mission (from the 2026-11-12 sweep), so a change to a
# controller's report format fails here instead of silently losing the win frame.
SAMPLE = {
    "t1": ("PASS Wasteland: frame=10120 ticks=10151 Marines=10", 10120),
    "t2": ("Backwater combat result: frame=29366 ticks=29387 won=1 lost=0 minerals=1164", 29366),
    "t3": ("T3 controller run won: ticks=54007 elapsed=54000 remaining=0", 54007),
    "t4": ("T4 attempt result=WON frame=8911 elapsed=8911 trace=x", 8911),
    "t5": ("T5 won tick=127883 frame=126331 result=WON switch4=0", 126331),
    "t6": ("T6 probe result ticks=23799 frame=23536 won=1 lost=0 raw19=1@goal1", 23536),
    "t7": ("T7 result tick=4189 frame=3931 won=1 lost=0 faults=0", 3931),
    "t8": ("mission=11 win=1 lost=0 tick=143260 frame=139216 faults=0 success=1", 139216),
    "t9": ("T9 attempt tick=47984 frame=47266 outcome=won ProtossBuildings=0", 47266),
    "t10": ("mission=13 win=1 lost=0 tick=3446 frame=3361 faults=0 success=1", 3361),
    "z1": ("Z1 controller replay completed at tick 50816: Hydralisks=30", 50816),
    "z2": ("Controller completed race mission 1 in 8412 input frames.", 8412),
    "z3": ("Z3 normal-controller run: policy=hybrid result=WON frame=136588 elapsed=136588", 136588),
    "z4": ("Z4 controller attempt result=WON frame=34984 host_tick=35186", 34984),
    "z5": ("Z5 controller attempt=VERIFIED-WIN frame=48347 result=1/0", 48347),
    "z6": ('{"kind":"won","host_tick":57562,"frame":56913,"screen":3}', 56913),
    "z7": ("Z7 controller result=won host_ticks=186921 frame=184248 model=1", 184248),
    "z8": ("z8 controller: frame=44150 calls=45187 won=1 lost=0 screen=3", 44150),
    "z9": ("z9 controller: frame=68520 calls=70321 won=1 lost=0 screen=3", 68520),
    "z10": ("z10 controller: frame=58795 calls=60834 won=1 lost=0 screen=3", 58795),
}
PX_LINE = ("PX CHECK won tick=94514 frame=92865 screen=3 won=1 lost=0 mission=27 faults=0", 92865)


class ListTests(unittest.TestCase):
    def test_exactly_the_thirty_missions(self) -> None:
        expected = [f"{race}{n}" for race, count in (("t", 10), ("z", 10), ("p", 10))
                    for n in range(1, count + 1)]
        self.assertEqual([m.key for m in cc.MISSIONS], expected)

    def test_win_patterns_read_real_output(self) -> None:
        for key, (line, frame) in SAMPLE.items():
            with self.subTest(mission=key):
                match = re.search(cc.BY_KEY[key].win, line)
                self.assertIsNotNone(match)
                self.assertEqual(int(match.group(1)), frame)
        for n in range(1, 11):
            match = re.search(cc.BY_KEY[f"p{n}"].win, PX_LINE[0])
            self.assertEqual(int(match.group(1)), PX_LINE[1])
        self.assertIsNone(re.search(cc.BY_KEY["p8"].win, PX_LINE[0].replace("won=1", "won=0")))

    def test_every_mission_has_a_rule_and_its_build_targets_in_the_make_database(self) -> None:
        rules = romcheck.read_rules([], ROOT)
        for mission in cc.MISSIONS:
            with self.subTest(mission=mission.key):
                self.assertIn(mission.rule, rules, f"{mission.rule} is not defined")
                for kind, targets in mission.steps:
                    for target in targets:
                        if kind == "build" or target.startswith("build/"):
                            self.assertIn(target, rules, f"{target} has no rule")
        # The check rule itself must depend on what the tool builds first.
        for mission in cc.MISSIONS:
            if not mission.key.startswith("p"):
                self.assertEqual(rules[mission.rule], list(mission.steps[0][1]), mission.key)
            else:
                self.assertEqual(rules[mission.rule], [f"build/protoss-px/{mission.key}/host-full.txt"])

    def test_make_include_and_top_level_size(self) -> None:
        makefile = (ROOT / "Makefile").read_text(encoding="utf-8")
        self.assertIn("include mk/campaign-check/Makefile", makefile)
        self.assertLess((ROOT / "Makefile").stat().st_size, 512 * 1024)
        self.assertRegex((ROOT / "mk/campaign-check/Makefile").read_text(encoding="utf-8"),
                         r"(?m)^check-campaigns:")


class Runner:
    def __init__(self, outputs: dict[str, str], fail: tuple[str, ...] = ()) -> None:
        self.outputs, self.fail, self.calls = outputs, fail, []

    def __call__(self, command, log, cwd) -> int:
        target = command[-1]
        self.calls.append(target)
        log.parent.mkdir(parents=True, exist_ok=True)
        log.write_text("$ make\n" + self.outputs.get(target, ""), encoding="utf-8")
        return 1 if target in self.fail else 0


class RunTests(unittest.TestCase):
    def run_one(self, key, runner, missing=None):
        td = tempfile.TemporaryDirectory()
        self.addCleanup(td.cleanup)
        return cc.run_mission(
            cc.BY_KEY[key], [], Path(td.name), root=Path(td.name), runner=runner, lock_path=None,
            min_free_bytes=0, free_bytes=lambda: 10 ** 12,
            find_missing=lambda t, a: (missing or {}).get(t[0], []))

    def test_win_is_reported_with_its_frame_and_command(self) -> None:
        out = {"check-campaign-t4": "./build/test-sc64-full-t4-run --limit 30000\n" + SAMPLE["t4"][0] + "\n"}
        row = self.run_one("t4", Runner(out))
        self.assertEqual((row["status"], row["frame"]), ("PASS", 8911))
        self.assertTrue(row["command"].startswith("./build/test-sc64-full-t4-run"))

    def test_nonzero_exit_fails_and_stops_the_mission(self) -> None:
        runner = Runner({"build/px-p8-controller": "tests/x.c:1: error: boom\n"},
                        fail=("build/px-p8-controller",))
        row = self.run_one("p8", runner)
        self.assertEqual(row["status"], "FAIL")
        self.assertEqual(runner.calls, ["build/px-p8-controller"])

    def test_exit_zero_without_a_win_report_fails(self) -> None:
        row = self.run_one("z9", Runner({"check-campaign-z9": "z9 controller: frame=26001 calls=1 won=0 lost=1\n"}))
        self.assertEqual(row["status"], "FAIL")
        self.assertIn("does not report a win", row["detail"])

    def test_missing_inputs_skip(self) -> None:
        runner = Runner({})
        row = self.run_one("z10", runner, missing={"build/test-sc64-z10-campaign-run": [".local/x.json"]})
        self.assertEqual(row["status"], "SKIPPED")
        self.assertEqual(runner.calls, [])
        self.assertEqual(cc.exit_code([row]), 0)
        self.assertEqual(cc.exit_code([row], require_all=True), 1)

    def test_protoss_runs_controller_tape_replay_then_check(self) -> None:
        runner = Runner({"check-campaign-p3": "grep ' won=1 ' x\n" + PX_LINE[0] + "\n"})
        row = self.run_one("p3", runner)
        self.assertEqual(runner.calls, ["build/px-p3-controller", "build/protoss-px/p3/sc64_px_tape_generated.h",
                                        "build/px-p3-replay", "check-campaign-p3"])
        self.assertEqual(row["status"], "PASS")

    def test_table_names_every_column(self) -> None:
        rows = [{"mission": "T4", "rule": "check-campaign-t4", "command": "./build/x", "status": "PASS",
                 "detail": "", "frame": 8911, "unit": "frame"},
                {"mission": "T3", "rule": "check-campaign-t3", "command": "", "status": "FAIL",
                 "detail": "boom", "frame": None, "unit": "tick"}]
        table = cc.render_table(rows, fast=True)
        for text in ("make rule", "win frame", "8,911", "FAIL (boom)", "unsanitized"):
            self.assertIn(text, table)
        self.assertEqual(cc.exit_code(rows), 1)


class FlavourTests(unittest.TestCase):
    def test_a_change_of_flavour_discards_binaries(self) -> None:
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            binary = root / "build" / "test-sc64-full-t4-run"
            binary.parent.mkdir(parents=True)
            binary.write_text("x", encoding="utf-8")
            cc.reset_for_flavour(cc.MISSIONS, fast=False, fresh=False, root=root)
            self.assertTrue(binary.exists(), "first run has no stamp: nothing to discard")
            cc.reset_for_flavour(cc.MISSIONS, fast=False, fresh=False, root=root)
            self.assertTrue(binary.exists())
            cc.reset_for_flavour(cc.MISSIONS, fast=True, fresh=False, root=root)
            self.assertFalse(binary.exists())


if __name__ == "__main__":
    unittest.main()
