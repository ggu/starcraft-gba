# SPDX-License-Identifier: GPL-3.0-or-later
"""Compile the real version selection without generated game inputs."""
import pathlib
import subprocess
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]


def version_regions():
    text = (ROOT / "starcraft/game.c").read_text()
    generation = text[text.index("#if defined(SC64_P7_INTERCEPTOR_FLIGHT_ENABLED)\n#define SC64_RACE_SAVE_GENERATION"):
                      text.index("/* Opt-in Z/P pools")]
    start = text.index("#if defined(SC64_CAMPAIGN_RACE) && SC64_CAMPAIGN_RACE == 0\n#define SC64_BASE_SAVE_VERSION")
    version = text[start:text.index("#ifndef MAX_BUILDINGS", start)]
    return generation, "#if 1\n" + version


class SaveTags(unittest.TestCase):
    def compile_check(self, flags, expected, generation=None, succeeds=True):
        gen, version = version_regions()
        source = (gen if generation is None else "#define SC64_RACE_SAVE_GENERATION " + generation + "\n") + version
        source += f"\n_Static_assert(SAVE_VERSION == {expected}u, \"unexpected save tag\");\n"
        result = subprocess.run(["clang", "-std=c11", "-fsyntax-only", "-x", "c", "-", *flags],
                                input=source, text=True, capture_output=True)
        self.assertEqual(result.returncode == 0, succeeds, result.stderr)
        if not succeeds:
            self.assertIn("base save tag overlaps roster save namespace", result.stderr)

    def test_four_canonical_tags_and_roster_namespace(self):
        profiles = [
            (["-DSC64_CAMPAIGN_RACE=0", "-DSC64_TERRAN_LATE_RUNTIME"], 0x53435442),
            (["-DSC64_CAMPAIGN_RACE=1", "-DSC64_Z6_TRANSPORT_ENABLED"], 0x53435A50),
            (["-DSC64_CAMPAIGN_RACE=2", "-DSC64_P7_INTERCEPTOR_FLIGHT_ENABLED"], 0x5343505B),
            (["-DSC64_CAMPAIGN_RACE=2", "-DSC64_P7_TYPED_COMPACT_ENABLED"], 0x53435058),
        ]
        tags = set()
        for flags, base in profiles:
            with self.subTest(flags=flags):
                self.compile_check(flags, base)
                self.compile_check(flags + ["-DSC64_SKIRMISH_ROSTER"], base | 0x80000000)
                tags.update((base, base | 0x80000000))
        self.assertEqual(len(tags), 8)

    def test_future_base_cannot_enter_reserved_namespace(self):
        self.compile_check(["-DSC64_CAMPAIGN_RACE=0", "-DSC64_SKIRMISH_ROSTER"],
                           0xD3435430, generation="0x80000000u", succeeds=False)


if __name__ == "__main__":
    unittest.main()
