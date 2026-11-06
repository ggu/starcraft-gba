# SPDX-License-Identifier: GPL-3.0-or-later
"""Fail-closed parsing for bounded Z7 attribution records."""
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import sc64_z7_busy_attribution as attribution


def diagnostic():
    lines = ["Z7A ROUTE slots=14 hits=200 builds=150 build_frames=100 "
             "no_build_frames=875 max_builds=3 build_update=500 no_build_update=3500"]
    for index in range(len(attribution.STAGES)):
        lines.append(f"Z7A STAGE index={index} sum=1000 peak=100")
    for rank in range(1, 11):
        lines.append(f"Z7A TOP rank={rank} total={110-rank} tick={136058+rank} "
                     "update=50 render=40 builds=1 hits=2")
    return "\n".join(lines) + "\n"


class AttributionTests(unittest.TestCase):
    def test_checks_required_records_and_frame_counts(self):
        text = diagnostic()
        parsed = attribution.parse_attribution(text, 136058, 975)
        self.assertEqual(len(parsed["STAGE"]), len(attribution.STAGES))
        for damaged in (text.replace(" builds=150", "", 1),
                        text.replace("build_frames=100", "build_frames=101", 1),
                        text.replace("Z7A STAGE index=3 sum=1000 peak=100\n", "", 1),
                        text.replace("rank=2 total=108", "rank=2 total=110", 1),
                        text.replace("index=0 sum=1000 peak=100", "index=0 sum=1000 sum=1000 peak=100", 1)):
            with self.assertRaises(ValueError):
                attribution.parse_attribution(damaged, 136058, 975)


if __name__ == "__main__":
    unittest.main()
