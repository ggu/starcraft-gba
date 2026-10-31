# SPDX-License-Identifier: GPL-3.0-or-later
"""Compile the real version selection without generated game inputs."""
import pathlib
import re
import subprocess
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]


def version_regions():
    text = (ROOT / "starcraft/game.c").read_text()
    lines = text.splitlines(keepends=True)
    first = next(i for i, line in enumerate(lines)
                 if re.match(r"^#define\s+SC64_RACE_SAVE_GENERATION\b", line))
    start = first - 1
    if not re.match(r"^#if\b", lines[start]):
        raise ValueError("save generation selection must start with an #if")
    depth = 0
    for end in range(start, len(lines)):
        if re.match(r"^#(?:if|ifdef|ifndef)\b", lines[end]):
            depth += 1
        elif re.match(r"^#endif\b", lines[end]):
            depth -= 1
            if not depth:
                break
    else:
        raise ValueError("unterminated save generation selection")
    generation = "".join(lines[start:end + 1])
    start = text.index("#if defined(SC64_CAMPAIGN_RACE) && SC64_CAMPAIGN_RACE == 0\n#define SC64_BASE_SAVE_VERSION")
    version = text[start:text.index("#ifndef MAX_BUILDINGS", start)]
    return generation, "#if 1\n" + version


class SaveTags(unittest.TestCase):
    def compile_check(self, flags, expected, generation=None, succeeds=True,
                      diagnostic="base save tag overlaps roster save namespace"):
        gen, version = version_regions()
        source = (gen if generation is None else "#define SC64_RACE_SAVE_GENERATION " + generation + "\n") + version
        source += f"\n_Static_assert(SAVE_VERSION == {expected}u, \"unexpected save tag\");\n"
        result = subprocess.run(["clang", "-std=c11", "-fsyntax-only", "-x", "c", "-", *flags],
                                input=source, text=True, capture_output=True)
        self.assertEqual(result.returncode == 0, succeeds, result.stderr)
        if not succeeds:
            self.assertIn(diagnostic, result.stderr)

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
                self.compile_check(flags + ["-DSC64_BATTLECRUISER_ENERGY_EXPERIMENT"],
                                   base | 0x20000000)
                self.compile_check(flags + ["-DSC64_SKIRMISH_ROSTER",
                                            "-DSC64_BATTLECRUISER_ENERGY_EXPERIMENT"],
                                   base | 0xA0000000)
                tags.update((base, base | 0x80000000, base | 0x20000000,
                             base | 0xA0000000))
        self.assertEqual(len(tags), 16)

    def test_z8_generation_precedes_other_profile_features(self):
        flags = ["-DSC64_CAMPAIGN_RACE=1", "-DSC64_Z8_RUNTIME",
                 "-DSC64_Z7_RUNTIME", "-DSC64_Z6_TRANSPORT_ENABLED",
                 "-DSC64_P7_INTERCEPTOR_FLIGHT_ENABLED"]
        self.compile_check(flags, 0x53435A5E)
        self.compile_check(flags + ["-DSC64_SKIRMISH_ROSTER",
                                    "-DSC64_BATTLECRUISER_ENERGY_EXPERIMENT"],
                           0xF3435A5E)

    def test_future_base_cannot_enter_reserved_namespace(self):
        self.compile_check(["-DSC64_CAMPAIGN_RACE=0", "-DSC64_SKIRMISH_ROSTER"],
                           0xD3435430, generation="0x80000000u", succeeds=False)

    def test_future_base_cannot_enter_bc_namespace(self):
        self.compile_check(["-DSC64_CAMPAIGN_RACE=0",
                            "-DSC64_BATTLECRUISER_ENERGY_EXPERIMENT"],
                           0x73435430, generation="0x20000000u", succeeds=False,
                           diagnostic="BC save namespace overlaps legacy tag")


if __name__ == "__main__":
    unittest.main()
