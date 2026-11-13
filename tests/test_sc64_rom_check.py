#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Asset-free tests for tools/sc64_rom_check.py (no ROMs, no make builds)."""

from __future__ import annotations

import re
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

import sc64_rom_check as rc  # noqa: E402


def makefile_texts() -> dict[str, str]:
    paths = [ROOT / "Makefile", *ROOT.glob("mk/*/Makefile"), *ROOT.glob("tests/*/Makefile"),
             *ROOT.glob("tests/*/*/Makefile"), *ROOT.glob("config/*/Makefile")]
    return {str(p.relative_to(ROOT)): p.read_text(encoding="utf-8") for p in paths}


def verify_rom_rules() -> dict[str, str]:
    """verify target -> ROM, for every rule whose prerequisites name a ROM."""
    found: dict[str, str] = {}
    pattern = re.compile(r"(?m)^(verify(?:-[A-Za-z0-9_-]+)?)\s*:(?!=)\s*([^\n#]*)")
    for text in makefile_texts().values():
        for target, prereqs in pattern.findall(text):
            roms = [p for p in prereqs.split() if p.startswith("build/") and p.endswith(".gba")]
            if roms:
                found[target] = roms[0]
    return found


class ListTests(unittest.TestCase):
    def test_every_verify_rom_rule_is_listed(self) -> None:
        listed = dict(rc.ROM_TARGETS)
        found = verify_rom_rules()
        self.assertGreater(len(found), 10, "the scan found too few verify rules")
        missing = sorted(set(found) - set(listed))
        self.assertEqual(missing, [], "verify rules missing from tools/sc64_rom_check.py "
                         "ROM_TARGETS (add them): " + ", ".join(missing))

    def test_every_listed_target_has_a_rule_for_that_rom(self) -> None:
        found = verify_rom_rules()
        for target, rom in rc.ROM_TARGETS:
            with self.subTest(target=target):
                self.assertEqual(found.get(target), rom)

    def test_no_duplicate_targets(self) -> None:
        names = [t for t, _ in rc.ROM_TARGETS]
        self.assertEqual(len(names), len(set(names)))

    def test_make_include_is_wired_in(self) -> None:
        makefile = (ROOT / "Makefile").read_text(encoding="utf-8")
        self.assertIn("include mk/rom-check/Makefile", makefile)
        self.assertRegex((ROOT / "mk/rom-check/Makefile").read_text(encoding="utf-8"),
                         r"(?m)^check-all-roms:")
        self.assertLess((ROOT / "Makefile").stat().st_size, 512 * 1024)


class Runner:
    def __init__(self, fail=()) -> None:
        self.fail, self.calls = fail, []

    def __call__(self, command, log, cwd) -> int:
        self.calls.append(command[-1])
        (cwd / "build").mkdir(exist_ok=True)
        (cwd / "build" / "x.elf").write_bytes(b"not an elf")
        log.parent.mkdir(parents=True, exist_ok=True)
        log.write_text("fake log\n", encoding="utf-8")
        return 1 if command[-1] in self.fail else 0


class CheckTests(unittest.TestCase):
    def run_check(self, targets, runner, missing=None, **kwargs):
        td = tempfile.TemporaryDirectory()
        self.addCleanup(td.cleanup)
        rules = {rom: ["build/x.elf"] for _, rom in targets}
        return rc.check_all(
            targets, [], Path(td.name) / "out", root=Path(td.name), runner=runner, rules=rules,
            free_bytes=lambda: 100 * rc.rel.GIB,
            find_missing=lambda t, a: (missing or {}).get(t[0], []), **kwargs)

    def test_failure_skip_and_alias(self) -> None:
        targets = [("verify-a", "build/A.gba"), ("verify-b", "build/B.gba"),
                   ("verify-c", "build/C.gba"), ("verify-a2", "build/A.gba")]
        runner = Runner(fail=("verify-b",))
        rows = self.run_check(targets, runner, missing={"verify-c": [".local/x.h"]})
        status = {r["target"]: r["status"] for r in rows}
        self.assertEqual(status, {"verify-a": "PASS", "verify-b": "FAIL",
                                  "verify-c": "SKIPPED", "verify-a2": "PASS"})
        self.assertEqual(runner.calls, ["verify-a", "verify-b"], "must continue past a failure")
        self.assertIn("missing: .local/x.h", rows[2]["detail"])
        self.assertEqual(rows[3]["detail"], "same ROM as verify-a")
        self.assertEqual(rc.exit_code(rows), 1)

    def test_skipped_is_not_a_failure_unless_required(self) -> None:
        rows = self.run_check([("verify-a", "build/A.gba")], Runner(),
                              missing={"verify-a": ["in"]})
        self.assertEqual(rc.exit_code(rows), 0)
        self.assertEqual(rc.exit_code(rows, require_all=True), 1)

    def test_low_disk_fails_without_building(self) -> None:
        td = tempfile.TemporaryDirectory()
        self.addCleanup(td.cleanup)
        runner = Runner()
        rows = rc.check_all([("verify-a", "build/A.gba")], [], Path(td.name), root=ROOT,
                            runner=runner, rules={}, free_bytes=lambda: 0,
                            find_missing=lambda t, a: [])
        self.assertEqual(rows[0]["status"], "FAIL")
        self.assertEqual(runner.calls, [])

    def test_table_has_margins_and_limits(self) -> None:
        rows = [{"target": "verify-x", "status": "PASS", "detail": "", "iwram_end": 0x03004F00,
                 "bss_end": 0x0203DF00, "iwram_left": 256, "ewram_left": 256}]
        table = rc.render_table(rows, {"__iwram_end": 0x03005000, "__bss_end": 0x0203E000})
        for text in ("verify-x", "0x03004f00", "0x0203df00", "256", "PASS", "0x0203e000"):
            self.assertIn(text, table)

    def test_select_rejects_unknown(self) -> None:
        with self.assertRaises(rc.rel.ReleaseError):
            rc.select(rc.ROM_TARGETS, "verify-nope")
        self.assertEqual([t for t, _ in rc.select(rc.ROM_TARGETS, "verify,verify-showcase")],
                         ["verify", "verify-showcase"])


if __name__ == "__main__":
    unittest.main()
