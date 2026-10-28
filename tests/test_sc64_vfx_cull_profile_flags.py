#!/usr/bin/env python3
"""Check canonical source-VFX cull flags without building or preparing assets."""

from __future__ import annotations

import os
import pathlib
import tempfile
import subprocess
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]
PROFILES = ("TERRAN", "ZERG", "PROTOSS", "SHOWCASE")
NON_DELIVERED = ("ZERG_Z4", "PROTOSS_P5", "PROTOSS_P6", "Z7_FULL_RENDER",
                 "P7_RENDER")
NON_DELIVERED_WITH_SOURCE = NON_DELIVERED[:-1]
CULL = "-DSC64_WEAPON_SOURCE_VFX_OFFSCREEN_CULL"
SOURCE = "-DSC64_WEAPON_SOURCE_VFX"


def expanded_profiles(*overrides: str) -> dict[str, str]:
    env = os.environ.copy()
    # Inject a dependency-free print target after the real Makefile. It has no
    # prerequisites or recipe, so expansion cannot invoke asset generators.
    variables = (("TERRAN", "TERRAN_ROM_FLAGS"),
                 ("ZERG", "ZERG_ROM_FLAGS"),
                 ("PROTOSS", "PROTOSS_ROM_FLAGS"),
                 ("SHOWCASE", "SHOWCASE_FLAGS"),
                 ("ZERG_Z4", "ZERG_Z4_ROM_FLAGS"),
                 ("PROTOSS_P5", "PROTOSS_P5_CANONICAL_FLAGS"),
                 ("PROTOSS_P6", "PROTOSS_P6_CANONICAL_FLAGS"),
                 ("Z7_FULL_RENDER", "Z7_FULL_RENDER_FLAGS"),
                 ("P7_RENDER", "P7_RENDER_FLAGS"),
                 ("CFLAGS", "CFLAGS"))
    with tempfile.TemporaryDirectory(prefix="sc64-vfx-cull-flags-") as directory:
        rules = pathlib.Path(directory) / "print.mk"
        rules.write_text(
            "".join(f"$(info VFX_CULL_PROFILE_{label}=$({variable}))\n"
                    for label, variable in variables)
            + ".PHONY: print-vfx-cull-profile-flags\n"
              "print-vfx-cull-profile-flags:\n",
            encoding="utf-8",
        )
        result = subprocess.run(
            ["make", "-n", "--no-print-directory", "-f", str(ROOT / "Makefile"),
             "-f", str(rules), "print-vfx-cull-profile-flags", "SC64_AUDIO=0",
             *overrides],
            cwd=ROOT,
            env=env,
            check=True,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
    parsed: dict[str, str] = {}
    for line in result.stdout.splitlines():
        if line.startswith("VFX_CULL_PROFILE_") and "=" in line:
            key, value = line.split("=", 1)
            parsed[key.removeprefix("VFX_CULL_PROFILE_")] = value
    return parsed


class VfxCullProfileFlagsTest(unittest.TestCase):
    def test_canonical_source_weapon_products_enable_cull(self) -> None:
        profiles = expanded_profiles()
        self.assertEqual(set(PROFILES) | set(NON_DELIVERED) | {"CFLAGS"},
                         set(profiles))
        for name in PROFILES:
            with self.subTest(profile=name):
                self.assertIn(SOURCE, profiles[name].split())
                self.assertIn(CULL, profiles[name].split())
        # The default original-art target links with CFLAGS only; imported
        # races flags are a separate product and must not stand in for it.
        self.assertNotIn(SOURCE, profiles["CFLAGS"].split())
        self.assertNotIn(CULL, profiles["CFLAGS"].split())

    def test_cull_override_can_disable_cull_for_products(self) -> None:
        profiles = expanded_profiles("CANONICAL_SOURCE_VFX_CULL_FLAGS=")
        for name in PROFILES:
            with self.subTest(profile=name):
                self.assertIn(SOURCE, profiles[name].split())
                self.assertNotIn(CULL, profiles[name].split())

    def test_weapon_override_without_source_renderer_removes_cull(self) -> None:
        profiles = expanded_profiles("CANONICAL_WEAPON_FLAGS=-DSC64_TANK_SOURCE_VFX")
        for name in PROFILES:
            with self.subTest(profile=name):
                self.assertNotIn(SOURCE, profiles[name].split())
                self.assertNotIn(CULL, profiles[name].split())

    def test_source_weapon_override_off_removes_cull(self) -> None:
        profiles = expanded_profiles("SOURCE_WEAPON_FLAGS=")
        for name in PROFILES:
            with self.subTest(profile=name):
                self.assertNotIn(SOURCE, profiles[name].split())
                self.assertNotIn(CULL, profiles[name].split())

    def test_unqualified_legacy_and_probe_profiles_keep_existing_flags(self) -> None:
        profiles = expanded_profiles()
        for name in NON_DELIVERED:
            with self.subTest(profile=name):
                if name in NON_DELIVERED_WITH_SOURCE:
                    self.assertIn(SOURCE, profiles[name].split())
                self.assertNotIn(CULL, profiles[name].split())


if __name__ == "__main__":
    unittest.main()
