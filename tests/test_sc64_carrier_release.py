# SPDX-License-Identifier: GPL-3.0-or-later
"""Asset-free checks for the opt-in Carrier release vector and target dispatch."""
import os
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import unittest


ROOT = Path(__file__).resolve().parents[1]
PROFILES = {
    "terran": ("TERRAN_ROM_FLAGS", "SC64_TERRAN_CARRIER_PROBE",
               "build/StarCraft-Terran-Carrier-Probe.gba", "test-sc64-terran-carrier-runtime"),
    "zerg-z10": ("ZERG_Z10_RELEASE_FLAGS", "SC64_ZERG_CARRIER_PROBE",
                 "build/StarCraft-Zerg-Z10-Carrier-Probe.gba", "test-sc64-zerg-carrier-runtime"),
    "showcase": ("SHOWCASE_FLAGS", "SC64_SHOWCASE_CARRIER_PROBE",
                 "build/StarCraft-Showcase-Carrier-Probe-GBA.gba", "test-sc64-showcase-carrier"),
    "protoss-p10": ("PROTOSS_P10_FLAGS", None,
                   "build/StarCraft-Protoss-P10-Transport1-GBA.gba", "test-sc64-p10-gantrithor"),
}
SERVICE = {
    "SC64_INTERCEPTOR_COMPACT_CODEC", "SC64_P7_PROJECTILES_ENABLED",
    "SC64_P7_CARRIER_TRAINING_ENABLED", "SC64_P7_INTERCEPTOR_FLIGHT_ENABLED",
}
PROBES = {gate for _, gate, _, _ in PROFILES.values() if gate}


def run(args, stdin=None):
    return subprocess.run(args, cwd=ROOT, text=True, input=stdin, capture_output=True)


class CarrierReleaseTest(unittest.TestCase):
    def setUp(self):
        self.cc = os.environ.get("CC") or shutil.which("clang")
        if not self.cc:
            self.skipTest("need a C preprocessor")

    def make(self, *args, stdin=None):
        return run(["make", "-s", "-f", "Makefile", "-f",
                    "mk/carrier-release/Makefile", *args], stdin)

    def release_flags(self, *names, settings=()):
        program = ".PHONY: print-carrier-flags\nprint-carrier-flags:\n"
        program += "".join(f"\t@printf '%s\\n' '$({name})'\n" for name in names)
        result = self.make("-f", "-", *settings, "print-carrier-flags", stdin=program)
        self.assertEqual(result.returncode, 0, result.stderr)
        return [shlex.split(line) for line in result.stdout.splitlines()]

    def macros(self, *flags):
        return run([self.cc, "-dM", "-E", "-x", "c", *flags, "-"], "")

    def test_opt_in_selects_exact_existing_profile_and_protoss_is_noop(self):
        names = [row[0] for row in PROFILES.values()]
        *profiles, selector = self.release_flags(*names, "SC64_CARRIER_RELEASE_FLAGS")
        for name, flags in zip(PROFILES, profiles):
            with self.subTest(profile=name):
                off = self.macros(*flags)
                on = self.macros(*flags, *selector)
                self.assertEqual(off.returncode, 0, off.stderr)
                self.assertEqual(on.returncode, 0, on.stderr)
                def names(text):
                    return set(re.findall(r"^#define (\w+)", text, re.M))
                old, new = names(off.stdout), names(on.stdout)
                self.assertEqual(old & PROBES, set())
                probe = PROFILES[name][1]
                extras = {"SC64_CARRIER_RELEASE", "SC64_CARRIER_RELEASE_H"}
                if probe:
                    expected = extras | {probe} | SERVICE
                    if name == "showcase":
                        expected.add("SC64_P7_RECT_TABLES")
                    self.assertEqual(new - old, expected - old)
                else:
                    self.assertEqual(new - old, extras)
                    self.assertTrue(SERVICE - {"SC64_INTERCEPTOR_COMPACT_CODEC"} <= old)
                self.assertEqual(old - new, set())

    def test_selector_rejects_unknown_ambiguous_and_mixed_profiles(self):
        [selector] = self.release_flags("SC64_CARRIER_RELEASE_FLAGS")
        for flags in ([], ["-DSC64_SHOWCASE_ENABLED", "-DSC64_Z10_RUNTIME",
                            "-DSC64_CAMPAIGN_RACE=1"],
                      ["-DSC64_SHOWCASE_ENABLED", "-DSC64_SHOWCASE_CARRIER_PROBE"]):
            with self.subTest(flags=flags):
                self.assertNotEqual(self.macros(*selector, *flags).returncode, 0)

    def test_normal_z10_preserves_fact_and_target_flags(self):
        names = ("Z10_CARRIER_FLAGS", "Z10_ARM_SOURCES", "Z10_FACT_FLAGS",
                 "Z10_FACT_REFERENCE_FLAGS")
        [selector] = self.release_flags("SC64_CARRIER_RELEASE_FLAGS")
        for policy in ("0", "1"):
            with self.subTest(target_policy=policy):
                shared = (f"CANONICAL_TARGET_POLICY={policy}",)
                off, stock_sources, stock_tables, stock_generator = self.release_flags(
                    *names, settings=(*shared, "Z10_CARRIER_RELEASE=0"))
                on, carrier_sources, carrier_tables, carrier_generator = self.release_flags(
                    *names, settings=(*shared, "Z10_CARRIER_RELEASE=1"))
                self.assertEqual(off, [])
                self.assertEqual(on, selector)
                model = "starcraft/sc64_interceptor_orders.c"
                self.assertEqual(carrier_sources.count(model), 1)
                self.assertEqual([source for source in carrier_sources if source != model],
                                 stock_sources)
                self.assertEqual(stock_tables, ["-DSC64_FACT_TABLES", "-Ibuild/generated/z10-facts"])
                self.assertEqual(carrier_tables,
                                 ["-DSC64_FACT_TABLES", "-Ibuild/generated/z10-facts-carrier"])
                self.assertEqual(carrier_generator, stock_generator + selector)
                self.assertIn("-DSC64_BUILDING_WORDS_PROBE", carrier_generator)
                self.assertIn("-DSC64_TARGET_FAST_LINK_PROBE", carrier_generator)
                self.assertEqual("-DSC64_TARGET_POLICY_PROBE" in carrier_generator, policy == "1")
        self.assertNotEqual(self.make("Z10_CARRIER_RELEASE=2", "carrier-release-plan").returncode, 0)

    def test_dispatch_maps_each_product_and_does_not_accept_unknowns(self):
        expected = [f"{name} {row[2]} {row[3]}" for name, row in PROFILES.items()]
        for name, lines in [("all", expected), *zip(PROFILES, ([line] for line in expected))]:
            with self.subTest(profile=name):
                result = self.make(f"SC64_CARRIER_RELEASE_PROFILE={name}", "carrier-release-plan")
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertEqual(result.stdout.splitlines(), lines)
        for bad in ("unknown", "", "terran showcase"):
            self.assertNotEqual(self.make(f"SC64_CARRIER_RELEASE_PROFILE={bad}",
                                          "carrier-release-plan").returncode, 0)


if __name__ == "__main__":
    unittest.main()
