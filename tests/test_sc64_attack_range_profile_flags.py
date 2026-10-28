"""Check which shipped profiles reserve a firing-region route field."""

import unittest

from test_sc64_vfx_cull_profile_flags import expanded_profiles, NON_DELIVERED, PROFILES


class AttackRangeProfileFlagsTest(unittest.TestCase):
    def test_only_current_products_reserve_the_combat_field(self):
        profiles = expanded_profiles()
        for name in PROFILES:
            with self.subTest(profile=name):
                self.assertIn("-DSC64_ROUTE_PACK3", profiles[name].split())
                self.assertIn("-DSC64_ATTACK_RANGE_ROUTES", profiles[name].split())
        for name in (*NON_DELIVERED, "CFLAGS"):
            with self.subTest(profile=name):
                self.assertNotIn("-DSC64_ATTACK_RANGE_ROUTES", profiles[name].split())

    def test_override_restores_point_route_capacity(self):
        profiles = expanded_profiles("CANONICAL_ATTACK_RANGE_FLAGS=")
        for name, flags in profiles.items():
            with self.subTest(profile=name):
                self.assertNotIn("-DSC64_ATTACK_RANGE_ROUTES", flags.split())


if __name__ == "__main__":
    unittest.main()
