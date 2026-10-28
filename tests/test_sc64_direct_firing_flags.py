#!/usr/bin/env python3
"""Check direct firing point expansion without building/importing assets."""

import unittest

from test_sc64_vfx_cull_profile_flags import expanded_profiles

FEATURE = "-DSC64_DIRECT_FIRING_POINTS"
DELIVERED = ("TERRAN", "ZERG", "PROTOSS", "SHOWCASE")
NON_DELIVERED = ("ZERG_Z4", "PROTOSS_P5", "PROTOSS_P6",
                 "Z7_FULL_RENDER", "P7_RENDER", "CFLAGS")


class DirectFiringProfileFlagsTest(unittest.TestCase):
    def test_enabled_only_for_four_delivered_profiles(self):
        profiles = expanded_profiles()
        for name in DELIVERED:
            with self.subTest(profile=name):
                self.assertIn(FEATURE, profiles[name].split())
        for name in NON_DELIVERED:
            with self.subTest(profile=name):
                self.assertNotIn(FEATURE, profiles[name].split())

    def test_empty_override_opts_out_of_all_delivered_profiles(self):
        profiles = expanded_profiles("CANONICAL_DIRECT_FIRING_FLAGS=")
        for name in DELIVERED:
            with self.subTest(profile=name):
                self.assertNotIn(FEATURE, profiles[name].split())


if __name__ == "__main__":
    unittest.main()
