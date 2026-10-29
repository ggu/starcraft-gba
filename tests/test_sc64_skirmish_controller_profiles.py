# SPDX-License-Identifier: GPL-3.0-or-later
#!/usr/bin/env python3
"""Verify controller build commands use the actual production profiles.

Make expands these rules through eval, so inspecting the variable names in the
Makefile alone cannot detect flags captured before their final definitions.
No generated assets or compiler are needed for these dry runs.
"""
from __future__ import annotations

import shlex
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PROFILES = {"terran": "TERRAN_ROM_FLAGS", "zerg": "ZERG_ROM_FLAGS",
            "protoss": "PROTOSS_ROM_FLAGS"}
INPUTS = ("TERRAN_FULL_INPUTS", "ZERG_Z6_INPUTS", "PROTOSS_CANONICAL_INPUTS",
          "PROTOSS_P7_CANONICAL_INPUTS", "UNIT_FACING_HEADER", "TANK_SOURCE_HEADER",
          "SOURCE_WEAPON_INPUTS", "CANONICAL_DEATH_INPUTS", "P7_ARBITER_INPUTS",
          "P7_FACT_V2_HEADER", "RACE_CAMPAIGN_INPUTS", "RACES_INPUTS")
MOTION = "starcraft/sc64_interceptor_motion.c"
CODEC = "starcraft/sc64_interceptor_pool_codec.c"
ORDERS = "starcraft/sc64_interceptor_orders.c"


class SkirmishControllerProfiles(unittest.TestCase):
    def test_canonical_p7_controller_source_and_dependencies(self) -> None:
        make = shutil.which("make")
        if not make:
            self.skipTest("make is required")
        target = "build/test-sc64-protoss-canonical-p7-controller"
        with tempfile.TemporaryDirectory(prefix="sc64-p7-make-") as td:
            helper = Path(td) / "inputs.mk"
            helper.write_text("print-p7-inputs:\n" + "".join(
                f"\t$(info SC64_INPUT\t{name}\t$({name}))\n" for name in INPUTS),
                encoding="utf-8")
            base = [make, "--no-print-directory", "-f", "Makefile", "-f", str(helper)]
            result = subprocess.run(base + ["-s", "print-p7-inputs"], cwd=ROOT,
                                    text=True, capture_output=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            values = dict(line.split("\t", 2)[1:] for line in result.stdout.splitlines()
                          if line.startswith("SC64_INPUT\t"))
            old = sorted({token for value in values.values() for token in shlex.split(value)})
            old_args = [arg for path in old for arg in ("-o", path)]
            controller = "tests/sc64_p7_campaign_run.c"
            dry = subprocess.run(base + ["-n", "-p", "-B", *old_args,
                                          "CLANG=SC64_TEST_CC", target],
                                 cwd=ROOT, text=True, capture_output=True)
            self.assertEqual(dry.returncode, 0, dry.stderr)
            commands = [shlex.split(line) for line in dry.stdout.splitlines()
                        if line.startswith("SC64_TEST_CC ")]
            self.assertEqual(len(commands), 1, dry.stdout)
            args = commands[0]
            for source in (controller, "common/gba.c", MOTION, CODEC, ORDERS):
                self.assertEqual(args.count(source), 1)
            self.assertNotIn("starcraft/game.c", args)
            dependencies = next(line for line in dry.stdout.splitlines()
                                if line.startswith(target + ":"))
            self.assertIn(controller, shlex.split(dependencies)[1:])

    def test_commands_use_late_production_vectors_and_companions(self) -> None:
        make = shutil.which("make")
        if not make:
            self.skipTest("make is required")
        with tempfile.TemporaryDirectory(prefix="sc64-controller-make-") as td:
            helper = Path(td) / "profiles.mk"
            # Appended after the repository Makefile is parsed. The unique flags
            # expose a recipe that captured a profile during an earlier eval.
            lines = [f"{var} += -DSC64_TEST_LATE_{name.upper()} -I/tmp/sc64-late-{name}"
                     for name, var in PROFILES.items()]
            lines += ["print-controller-values:"]
            for name in (*PROFILES.values(), *INPUTS):
                lines.append(f"\t$(info SC64_VALUE\t{name}\t$({name}))")
            helper.write_text("\n".join(lines) + "\n", encoding="utf-8")
            base = [make, "--no-print-directory", "-f", "Makefile", "-f", str(helper)]
            result = subprocess.run(base + ["-s", "print-controller-values"], cwd=ROOT,
                                    text=True, capture_output=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            values = dict(line.split("\t", 2)[1:] for line in result.stdout.splitlines()
                          if line.startswith("SC64_VALUE\t"))
            # Treat prepared inputs as old so -n cannot traverse their generation
            # rules or require the private source assets behind them.
            old = sorted({token for name in INPUTS for token in shlex.split(values[name])})
            old_args = [arg for path in old for arg in ("-o", path)]
            for profile, var in PROFILES.items():
                for fixture, source in (("build-controller", "tests/sc64_skirmish_build_controller.c"),
                    ("match", "tests/sc64_skirmish_campaign_run.c")):
                    with self.subTest(profile=profile, fixture=fixture):
                        target = f"build/test-sc64-skirmish-{fixture}-{profile}"
                        dry = subprocess.run(base + ["-n", "-B", *old_args,
                                                      "CLANG=SC64_TEST_CC", target],
                                             cwd=ROOT, text=True, capture_output=True)
                        self.assertEqual(dry.returncode, 0, dry.stderr)
                        commands = [shlex.split(line) for line in dry.stdout.splitlines()
                                    if line.startswith("SC64_TEST_CC ")]
                        self.assertEqual(len(commands), 1, dry.stdout)
                        args = commands[0]
                        actual = [a for a in args if (a.startswith("-D") or a.startswith("-I"))
                                  and a not in ("-DGBA_HOST", "-Icommon")]
                        expected = [a for a in shlex.split(values[var])
                                    if a.startswith("-D") or a.startswith("-I")]
                        self.assertEqual(actual, expected)
                        self.assertIn(f"-DSC64_TEST_LATE_{profile.upper()}", actual)
                        self.assertIn(source, args)
                        self.assertEqual(args.count(MOTION), 1)
                        self.assertEqual(args.count(CODEC), 1)
                        self.assertEqual(args.count(ORDERS), int(profile == "protoss"))
                        self.assertNotIn("starcraft/game.c", args)


if __name__ == "__main__":
    unittest.main()
