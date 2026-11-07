# SPDX-License-Identifier: GPL-3.0-or-later
"""Asset-free compile-time boundary for the isolated Q8 Flash stage."""
import subprocess
import unittest

FLAGS = ["GBA_HOST", "SC64_EXPERIMENTAL_RESEARCH_Q8_COMBAT",
         "SC64_EXPERIMENTAL_RESEARCH_JOBS_JOURNAL", "SC64_P7_TYPED_COMPACT_ENABLED",
         "SC64_P10_RUNTIME"]
ARM_FLAGS = ["SC64_RACES", "SC64_RESEARCH_BOUNDED_ARM_PROBE",
             "SC64_EXPERIMENTAL_RESEARCH_Q8_COMBAT",
             "SC64_EXPERIMENTAL_RESEARCH_JOBS_JOURNAL",
             "SC64_Q8_FLASH_ARM_PROBE"]

class Scope(unittest.TestCase):
    def preprocess(self, flags):
        return subprocess.run(["clang", "-E", "-x", "c", "-Istarcraft",
                               *("-D" + x for x in flags), "-"],
                              input='#include "sc64_q8_flash_scope.h"\n',
                              text=True, capture_output=True, check=False)

    def test_disabled_and_p10_enabled(self):
        self.assertEqual(self.preprocess([]).returncode, 0)
        self.assertEqual(self.preprocess(FLAGS + ["SC64_Q8_FLASH_HOST_STAGE"]).returncode, 0)

    def test_all_custom_flag_is_explicit_host_only(self):
        self.assertEqual(self.preprocess(FLAGS + ["SC64_Q8_FLASH_HOST_STAGE",
                                                   "SC64_Q8_FLASH_ALL_CUSTOM_HOST"]).returncode, 0)
        self.assertNotEqual(self.preprocess(["SC64_Q8_FLASH_ALL_CUSTOM_HOST"]).returncode, 0)

    def test_all_custom_arm_requires_explicit_transaction_probe(self):
        flag = "SC64_Q8_FLASH_ALL_CUSTOM_ARM_PROBE"
        self.assertEqual(self.preprocess(ARM_FLAGS + [flag]).returncode, 0)
        for missing in ARM_FLAGS:
            self.assertNotEqual(self.preprocess([x for x in ARM_FLAGS if x != missing] + [flag]).returncode, 0)
        for extra in ("GBA_HOST", "SC64_Q8_FLASH_HOST_STAGE",
                      "SC64_Q8_FLASH_ALL_CUSTOM_HOST",
                      "SC64_EXPERIMENTAL_RESEARCH_CAMPAIGN_JOBS_JOURNAL"):
            self.assertNotEqual(self.preprocess(ARM_FLAGS + [flag, extra]).returncode, 0)
        self.assertNotEqual(self.preprocess(FLAGS + ["SC64_Q8_FLASH_HOST_STAGE", flag]).returncode, 0)

    def test_non_p10_and_campaign_rejected(self):
        for flags in ([x for x in FLAGS if x != "SC64_P10_RUNTIME"],
                      FLAGS + ["SC64_EXPERIMENTAL_RESEARCH_CAMPAIGN_JOBS_JOURNAL"]):
            result = self.preprocess(flags + ["SC64_Q8_FLASH_HOST_STAGE"])
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("Q8 Flash stage requires HOST P10", result.stderr)

if __name__ == "__main__":
    unittest.main()
