"""Detached combined actor generation must not alias any audited old tag."""
import itertools
import json
from pathlib import Path
import unittest

from tests.test_sc64_guardian_tag_registry import FIXTURE, catalog, extract


class SharedActorTags(unittest.TestCase):
    def test_new_generation_is_disjoint_from_audited_formats(self):
        bases, generations = set(), set()
        for record in json.loads(FIXTURE.read_text()).values():
            found_bases, found_generations = catalog(record["definitions"])
            bases |= found_bases
            generations |= found_generations
        current_bases, current_generations = catalog(extract(Path("starcraft/game.c").read_text()))
        self.assertTrue(current_bases <= bases and current_generations <= generations)
        # Include roster, BC, cargo, transport, old Scarab, Guardian and Egg
        # selectors in every combination; this deliberately over-approximates
        # the historical products and experimental HOST-only namespaces.
        selectors = (0x80000000, 0x20000000, 0x01000000, 0x04000000,
                     0x08000000, 0x10000000, 0x40000000)
        old = set()
        for base, enabled in itertools.product(bases, itertools.product((False, True), repeat=len(selectors))):
            value = base
            for use, selector in zip(enabled, selectors):
                if use:
                    value ^= selector
            old.add(value)
        new_selector = 0x02000000
        self.assertTrue(all(tag & new_selector for tag in old))
        combined = {tag ^ new_selector for tag in old}
        self.assertFalse(old & combined)
        # Feature=5 and feature=7 are distinct even after considering every
        # actual base, BC/roster/cargo selector and outer transport form.
        base_selectors = (0x80000000, 0x20000000, 0x01000000, 0x04000000)
        feature5 = set()
        for base, enabled in itertools.product(bases, itertools.product((False, True), repeat=len(base_selectors))):
            value = base ^ new_selector
            for use, selector in zip(enabled, base_selectors):
                if use:
                    value ^= selector
            feature5.add(value)
        feature7 = {tag ^ 0x08000000 for tag in feature5}
        feature15 = {tag ^ 0x10000000 for tag in feature7}
        self.assertFalse(feature5 & feature7)
        self.assertFalse(feature5 & feature15)
        self.assertFalse(feature7 & feature15)
        self.assertFalse(old & feature5)
        self.assertFalse(old & feature7)
        self.assertFalse(old & feature15)
        source = Path("starcraft/game.c").read_text()
        self.assertIn("#define SC64_SHARED_ACTOR_SAVE_VERSION (SAVE_VERSION ^ 0x02000000u)", source)
        self.assertIn("#define SC64_SHARED_ACTOR_TRANSPORT_SAVE_VERSION (SC64_TRANSPORT_SAVE_VERSION ^ 0x02000000u)", source)
        self.assertIn("#define SC64_SHARED_ACTOR_FULL_SAVE_VERSION (SC64_SHARED_ACTOR_SAVE_VERSION ^ 0x08000000u)", source)
        self.assertIn("#define SC64_SHARED_ACTOR_FULL_TRANSPORT_SAVE_VERSION (SC64_SHARED_ACTOR_TRANSPORT_SAVE_VERSION ^ 0x08000000u)", source)
        self.assertIn("#define SC64_SHARED_ACTOR_FLIGHT_SAVE_VERSION (SC64_SHARED_ACTOR_FULL_SAVE_VERSION ^ 0x10000000u)", source)
        self.assertIn("#define SC64_SHARED_ACTOR_FLIGHT_TRANSPORT_SAVE_VERSION (SC64_SHARED_ACTOR_FULL_TRANSPORT_SAVE_VERSION ^ 0x10000000u)", source)
        print(f"shared actor tag audit: {len(old)} old values, {len(combined)} new values, no overlap")


if __name__ == "__main__":
    unittest.main()
