#!/usr/bin/env python3
"""Check shared engine mechanics in real production Make profiles.

This test intentionally compiles only the asset-free capability contract. It
uses the repository Makefile to expand production flag variables, rather than
duplicating their contents or building targets that require private assets.
"""

from __future__ import annotations

import os
import shlex
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
MAKE_VARS = (
    "TERRAN_ROM_FLAGS",
    "ZERG_ROM_FLAGS",
    "PROTOSS_P7_CANONICAL_FLAGS",
    "SHOWCASE_FLAGS",
    "RACES_FLAGS",
    "UNIFIED_FLAGS",
    # Older chapter and canonical products inherit the same engine contract.
    "T3_FLAGS",
    "Z3_FLAGS",
    "P3_FLAGS",
    "Z4_FLAGS",
    "TERRAN_FULL_FLAGS",
    "ZERG_Z4_ROM_FLAGS",
    "PROTOSS_P6_CANONICAL_FLAGS",
)

PROBE_SOURCE = r"""
#include "sc64_engine_capabilities.h"

#ifndef SC64_ENGINE_CORE_REQUIRED
#error "production profile omitted the shared-engine contract marker"
#endif
#ifndef SC64_RACES
#error "production profile omitted the shared race engine"
#endif

_Static_assert(SC64_ENGINE_CORE_CAPABILITIES == SC64_ENGINE_CORE_EXPECTED,
               "production profile is missing a shared core mechanic");
int sc64_engine_capability_probe(void) { return (int)SC64_ENGINE_CORE_CAPABILITIES; }
"""


class EngineCapabilityProfiles(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.make = shutil.which("make")
        cls.clang = shutil.which(os.environ.get("CLANG", "clang"))
        if not cls.make:
            raise unittest.SkipTest("make is required to expand production profiles")
        if not cls.clang:
            raise unittest.SkipTest("clang is required for the asset-free probes")

    def _make_values(self, overrides: tuple[str, ...] = ()) -> dict[str, str]:
        with tempfile.TemporaryDirectory(prefix="sc64-engine-capabilities-") as td:
            helper = Path(td) / "print-profiles.mk"
            recipe = ["print-engine-profiles:"]
            for name in (*MAKE_VARS, "RACE_CONSTRUCTION_INPUTS"):
                recipe.append(f"\t@printf '%s\\t%s\\n' '{name}' '$({name})'")
            helper.write_text("\n".join(recipe) + "\n", encoding="utf-8")
            result = subprocess.run(
                [self.make, "--no-print-directory", "-s", "-f", "Makefile",
                 "-f", str(helper), *overrides, "print-engine-profiles"],
                cwd=ROOT,
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                check=False,
            )
        self.assertEqual(
            result.returncode,
            0,
            msg=f"production Makefile expansion failed:\n{result.stderr}",
        )
        values: dict[str, str] = {}
        for line in result.stdout.splitlines():
            name, separator, value = line.partition("\t")
            self.assertTrue(separator, f"malformed Make output line: {line!r}")
            self.assertNotIn(name, values, f"duplicate Make variable {name}")
            values[name] = value
        self.assertEqual(set(values), {*MAKE_VARS, "RACE_CONSTRUCTION_INPUTS"})
        # The repository Makefile has pre-existing duplicate recipes. Their
        # warnings are captured above; only command status/output
        # affects this asset-free variable-expansion test.
        return values

    def _compile_probe(self, flags: str, *extra: str) -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            [self.clang, "-std=c11", "-Wall", "-Wextra", "-Werror", "-fsyntax-only",
             "-Istarcraft", *shlex.split(flags), *extra, "-x", "c", "-"],
            cwd=ROOT,
            input=PROBE_SOURCE,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            check=False,
        )

    def test_real_production_profiles_compile_with_shared_mechanics(self) -> None:
        values = self._make_values()
        for name in MAKE_VARS:
            with self.subTest(profile=name):
                args = shlex.split(values[name])
                self.assertIn("-DSC64_ENGINE_CORE_REQUIRED", args)
                self.assertIn("-DSC64_CONSTRUCTION_HP", args)
                self.assertIn("-DSC64_SIEGE_SPLASH", args)
                self.assertIn("-DSC64_SHARED_BATTERY", args)
                result = self._compile_probe(values[name])
                self.assertEqual(result.returncode, 0, result.stderr)

    def test_contract_rejects_missing_marker_or_either_mechanic(self) -> None:
        flags = self._make_values()["RACES_FLAGS"]
        for removal in (
            "-USC64_ENGINE_CORE_REQUIRED",
            "-USC64_CONSTRUCTION_HP",
            "-USC64_SIEGE_SPLASH",
            "-USC64_SHARED_BATTERY",
        ):
            with self.subTest(removed=removal):
                result = self._compile_probe(flags, removal)
                self.assertNotEqual(result.returncode, 0)

    def test_partial_shared_flag_override_fails_at_contract(self) -> None:
        values = self._make_values(("SC64_SHARED_ENGINE_FLAGS=-DSC64_CONSTRUCTION_HP",))
        result = self._compile_probe(values["RACES_FLAGS"])
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Production race-engine builds must include every shared core mechanic",
                      result.stderr)

    def test_original_construction_art_profile_keeps_engine_contract_without_art_inputs(self) -> None:
        values = self._make_values(("SC64_CONSTRUCTION_ART=0",))
        flags = shlex.split(values["RACES_FLAGS"])
        self.assertIn("-DSC64_ENGINE_CORE_REQUIRED", flags)
        self.assertIn("-DSC64_CONSTRUCTION_HP", flags)
        self.assertIn("-DSC64_SIEGE_SPLASH", flags)
        self.assertIn("-DSC64_SHARED_BATTERY", flags)
        self.assertNotIn("-DSC64_CONSTRUCTION_SOURCE_ART", flags)
        self.assertFalse(any(".local/sc64/construction/generated" in arg for arg in flags))
        self.assertNotIn("sc64_construction_sprites_generated.h",
                         values["RACE_CONSTRUCTION_INPUTS"])
        result = self._compile_probe(values["RACES_FLAGS"])
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_showcase_siege_override_is_explicitly_retired(self) -> None:
        with tempfile.TemporaryDirectory(prefix="sc64-retired-showcase-override-") as td:
            helper = Path(td) / "print-showcase.mk"
            helper.write_text(
                "print-showcase:\n\t@printf '%s\\n' '$(SHOWCASE_FLAGS)'\n",
                encoding="utf-8",
            )
            result = subprocess.run(
                [self.make, "--no-print-directory", "-s", "-f", "Makefile", "-f",
                 str(helper), "SHOWCASE_SIEGE_FLAGS=-DSC64_SIEGE_SPLASH", "print-showcase"],
                cwd=ROOT,
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                check=False,
            )
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("SHOWCASE_SIEGE_FLAGS is retired", result.stderr)


if __name__ == "__main__":
    unittest.main()
