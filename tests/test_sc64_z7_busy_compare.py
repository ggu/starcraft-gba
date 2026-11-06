# SPDX-License-Identifier: GPL-3.0-or-later
"""Fail-closed parsing for bounded Z7 diagnostic records."""
import copy
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import sc64_z7_busy_compare as probe


def diagnostic():
    lines = ["Z7B COUNTS calls=1000 advancing=975 text_before=0 text_after=0 "
             "n0=975 sum0=1950 peak0=2 n1=0 sum1=0 peak1=0 "
             "n2=25 sum2=75 peak2=3 high50=2 high10=3"]
    for index in range(4):
        play = 1000 if index == 1 else 0
        normal = 1000 if index == 0 else 0
        lines.append(f"Z7B MODAL index={index} screen_before={play} screen_after={play} "
                     f"mode_before={normal} mode_after={normal}")
    for index in range(10):
        lines.append(f"Z7B WORK index={index} first=10 last=11" +
                     (" minimum=9" if index < 4 else ""))
    for index in range(10):
        lines.append(f"Z7B SAMPLE tick={136058 + 100 * (index + 1)} state=42 pixels_palette=43")
    return "\n".join(lines + ["Z7B DONE"]) + "\n"


class BusyCompareTests(unittest.TestCase):
    def test_rejects_missing_hash_and_incoherent_counters(self):
        text = diagnostic()
        valid = probe.busy(text, 136058)
        self.assertEqual(len(valid["SAMPLE"]), 10)
        for damaged in (text.replace(" pixels_palette=43", "", 1),
                        text.replace("advancing=975", "advancing=0", 1),
                        text.replace("n2=25", "n2=24", 1),
                        text.replace("state=42", "state=42 state=42", 1)):
            with self.assertRaises(RuntimeError):
                probe.busy(damaged, 136058)

    def test_pair_checks_scene_and_hashes_but_allows_timing_difference(self):
        a = probe.busy(diagnostic(), 136058)
        b = copy.deepcopy(a)
        b["COUNTS"][0]["sum0"] += 1
        probe.compare_pair(a, b)
        b["SAMPLE"][0]["pixels_palette"] ^= 1
        with self.assertRaisesRegex(RuntimeError, "SAMPLE"):
            probe.compare_pair(a, b)


if __name__ == "__main__":
    unittest.main()
