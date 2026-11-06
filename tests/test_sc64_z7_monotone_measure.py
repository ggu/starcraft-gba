# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from sc64_z7_monotone_measure import parse_startup


class MonotoneMeasureTests(unittest.TestCase):
    def test_startup_and_stack_records_are_required_and_consistent(self):
        counts = {"sum0": 1950, "sum1": 0, "sum2": 75}
        text = ("Z7B STARTUP load=100 first100_sum=200 first100_peak=3 later900_sum=1825\n"
                "Z7B STACK painted=1000 untouched=500\n")
        self.assertEqual(parse_startup(text, counts)["stack"]["untouched"], 500)
        for bad in (text.replace("load=100", "load=0"),
                    text.replace("later900_sum=1825", "later900_sum=1824"),
                    text.replace("untouched=500", "untouched=0"),
                    text.replace("Z7B STACK", "Z7B IGNORED")):
            with self.assertRaises(ValueError):
                parse_startup(bad, counts)


if __name__ == "__main__":
    unittest.main()
