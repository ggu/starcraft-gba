# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import sc64_z7_route_classification as classification


class RouteClassificationTests(unittest.TestCase):
    def test_counts_are_complete_and_fail_closed(self):
        text = ("Z7C CLASS movement=300 component=20 depot=72 empty=10 evicted=382 "
                "resets=2 wraps=0 distinct=60 repeated=332 untracked=0 usable_min=12 usable_max=14\n")
        result = classification.parse_classification(text, {"builds": 392, "slots": 14})
        self.assertEqual(result["movement"], 300)
        for bad in (text.replace("depot=72", "depot=71"),
                    text.replace("evicted=382", "evicted=381"),
                    text.replace("untracked=0", "untracked=1"),
                    text.replace("usable_max=14", "usable_max=15"),
                    text.replace("wraps=0", "wraps=0 wraps=0")):
            with self.assertRaises(ValueError):
                classification.parse_classification(bad, {"builds": 392, "slots": 14})


if __name__ == "__main__":
    unittest.main()
