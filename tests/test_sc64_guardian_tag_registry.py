#!/usr/bin/env python3
"""Conservative journal collision check; optional pinned-source re-audit."""
import hashlib
import itertools
import json
import os
from pathlib import Path
import re
import subprocess
import unittest

FIXTURE = Path(__file__).with_name("sc64_guardian_tag_sources.json")
SELECTORS = (0x80000000, 0x20000000, 0x01000000, 0x04000000, 0x08000000)
GUARDIAN_XOR = 0x10000000
BASE = re.compile(r"(?:\(?(0x[0-9a-fA-F]{8})u \+ SC64_RACE_SAVE_GENERATION\)?|(0x[0-9a-fA-F]{8})u)$")
GEN = re.compile(r"(\d+)u$")
EXPR = re.compile(r"(?:[A-Z0-9_]+|\([A-Z0-9_]+ [|^] 0x[0-9a-fA-F]{8}u\))$")


def extract(data):
    return [s.strip() for s in data.splitlines() if re.match(
        r"#define (?:SC64_RACE_SAVE_GENERATION|SC64_BASE_SAVE_VERSION|SC64_LEGACY_SAVE_VERSION|SC64_PRE_CARGO_SAVE_VERSION|SC64_WORKER_CARGO_LEGACY_SAVE_VERSION|SC64_WORKER_CARGO_LEGACY_TRANSPORT_SAVE_VERSION|SAVE_VERSION|SC64_TRANSPORT_SAVE_VERSION|SC64_SCARAB_SAVE_VERSION|SC64_SCARAB_TRANSPORT_SAVE_VERSION|SC64_SHARED_CARRIER_SAVE_VERSION|SC64_SHARED_CARRIER_COMPACT_SAVE_VERSION)\b", s)]


def catalog(definitions):
    bases, gens = set(), set()
    for line in definitions:
        _, name, expr = line.split(None, 2)
        if name == "SC64_RACE_SAVE_GENERATION":
            match = GEN.fullmatch(expr)
            if not match or not 2 <= int(match[1]) <= 50:
                raise AssertionError("unrecognized generation: " + line)
            gens.add(int(match[1]))
        elif name == "SC64_BASE_SAVE_VERSION":
            match = BASE.fullmatch(expr)
            if not match:
                raise AssertionError("unrecognized base arithmetic: " + line)
            base = int(match[1] or match[2], 16)
            bases.update(base + n for n in range(2, 51)) if match[1] else bases.add(base)
        else:
            if not EXPR.fullmatch(expr):
                raise AssertionError("unrecognized selector arithmetic: " + line)
            for value in re.findall(r"0x[0-9a-fA-F]{8}", expr):
                if int(value, 16) not in SELECTORS:
                    raise AssertionError("unrecognized selector: " + line)
    if not bases or not gens:
        raise AssertionError("incomplete tag source")
    return bases, gens


class GuardianTags(unittest.TestCase):
    def test_registry(self):
        records = json.loads(FIXTURE.read_text())
        bases, gens = set(), set()
        for ref, record in records.items():
            b, g = catalog(record["definitions"]); bases |= b; gens |= g
            if os.environ.get("SC64_GUARDIAN_REAUDIT"):
                data = subprocess.check_output(["git", "show", ref + ":starcraft/game.c"])
                self.assertEqual(hashlib.sha256(data).hexdigest(), record["sha256"])
                self.assertEqual(extract(data.decode()), record["definitions"])
        # Also fail closed if this checkout introduces unfamiliar expressions.
        b, g = catalog(extract(Path("starcraft/game.c").read_text()))
        self.assertTrue(b <= bases and g <= gens, "current source extends fixture; refresh and re-audit")
        old = set()
        for base, bits in itertools.product(bases, itertools.product((False, True), repeat=len(SELECTORS))):
            value = base
            for enabled, flag in zip(bits, SELECTORS):
                if enabled: value ^= flag
            old.add(value)
        proposed = {tag ^ GUARDIAN_XOR for tag in old}
        self.assertFalse(old & proposed)
        self.assertTrue(all(tag & GUARDIAN_XOR for tag in old))
        print("Guardian registry:", len(records), "source snapshots,", len(old), "historical values, 0 collisions")

    def test_unknowns_fail_closed(self):
        for line in ("#define SC64_BASE_SAVE_VERSION (0x53435030u - SC64_RACE_SAVE_GENERATION)",
                     "#define SAVE_VERSION (SC64_LEGACY_SAVE_VERSION | 0x10000000u)"):
            with self.assertRaises(AssertionError): catalog(["#define SC64_RACE_SAVE_GENERATION 2u", line])


if __name__ == "__main__": unittest.main()
