# SPDX-License-Identifier: GPL-3.0-or-later
"""Asset-free compile-time boundary for the isolated Q8 Flash stage."""
import subprocess
import unittest

FLAGS = ["GBA_HOST", "SC64_EXPERIMENTAL_RESEARCH_Q8_COMBAT",
         "SC64_EXPERIMENTAL_RESEARCH_JOBS_JOURNAL", "SC64_P7_TYPED_COMPACT_ENABLED",
         "SC64_P10_RUNTIME"]

class Scope(unittest.TestCase):
    def preprocess(self, flags):
        return subprocess.run(["clang", "-E", "-x", "c", "-Istarcraft",
                               *("-D" + x for x in flags), "-"],
                              input='#include "sc64_q8_flash_scope.h"\n',
                              text=True, capture_output=True, check=False)

    def test_disabled_and_p10_enabled(self):
        self.assertEqual(self.preprocess([]).returncode, 0)
        self.assertEqual(self.preprocess(FLAGS + ["SC64_Q8_FLASH_HOST_STAGE"]).returncode, 0)

    def test_non_p10_and_campaign_rejected(self):
        for flags in ([x for x in FLAGS if x != "SC64_P10_RUNTIME"],
                      FLAGS + ["SC64_EXPERIMENTAL_RESEARCH_CAMPAIGN_JOBS_JOURNAL"]):
            result = self.preprocess(flags + ["SC64_Q8_FLASH_HOST_STAGE"])
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("Q8 Flash stage requires HOST P10", result.stderr)

if __name__ == "__main__":
    unittest.main()
