#!/usr/bin/env python3
"""Conservative source-derived journal namespace collision certificate.

It enumerates a superset of possible selector combinations, including illegal
ones; passing does not authorize any profile combination or journal reader.
"""
import itertools
import re
import subprocess
import unittest

REFS = (
    "9f596ad1", "1ffa7aad",
    "300ba3e512471d647f20824ad3f84765366cc560",  # Defiler/carrier draft
    "ef34c9c9d52b77b6849d134cbbdb19bd7cccb53b",  # Carrier stage
    "732eb074d6cbd131c5e4e0923ccf2f2d65fe7842",  # ordered Z10
)
SELECTORS = (0x80000000, 0x20000000, 0x01000000, 0x04000000, 0x08000000)
GUARDIAN_XOR = 0x10000000


def source(ref):
    return subprocess.check_output(["git", "show", ref + ":starcraft/game.c"], text=True)


def catalog():
    bases, gens, selectors = set(), set(), set()
    for ref in REFS:
        data = source(ref)
        gens.update(int(x) for x in re.findall(r"#define SC64_RACE_SAVE_GENERATION (\d+)u", data))
        for expr in re.findall(r"#define SC64_BASE_SAVE_VERSION ([^\n]+)", data):
            values = re.findall(r"0x([0-9a-fA-F]{8})u", expr)
            if len(values) != 1:
                raise AssertionError((ref, expr))
            base = int(values[0], 16)
            if "SC64_RACE_SAVE_GENERATION" in expr:
                bases.update(base + n for n in range(2, 51))
            else:
                bases.add(base)
        for line in data.splitlines():
            if line.startswith("#define ") and "SC64_BASE_SAVE_VERSION" not in line.split(" ")[1] and ("SAVE_VERSION" in line or "SAVE_TAG" in line):
                selectors.update(int(x, 16) for x in re.findall(r"0x([0-9a-fA-F]{8})u", line)
                                 if int(x, 16) and not (int(x, 16) & (int(x, 16) - 1)))
    if not gens or max(gens) > 50:
        raise AssertionError("generation range needs review")
    unknown = selectors - set(SELECTORS)
    if unknown:
        raise AssertionError("new tag selector requires review: " + repr(unknown))
    return bases, gens


class GuardianTags(unittest.TestCase):
    def test_conservative_cross_product(self):
        bases, gens = catalog()
        old = set()
        for base, bits in itertools.product(bases, itertools.product((False, True), repeat=len(SELECTORS))):
            value = base
            for enabled, flag in zip(bits, SELECTORS):
                if enabled:
                    value ^= flag  # union of OR/XOR selector outcomes, including forbidden combinations
            old.add(value)
        proposed = {tag ^ GUARDIAN_XOR for tag in old}
        self.assertFalse(old & proposed, "Guardian selector collides with a source-derived historical tag")
        self.assertTrue(all(tag & GUARDIAN_XOR for tag in old))
        print("Guardian registry: sources", len(REFS), "generations", len(gens),
              "bases", len(bases), "historical superset", len(old),
              "proposed", len(proposed), "collisions 0")


if __name__ == "__main__":
    unittest.main()
