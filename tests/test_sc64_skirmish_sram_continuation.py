# SPDX-License-Identifier: GPL-3.0-or-later
from __future__ import annotations

import copy
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import sc64_skirmish_sram_continuation as sram  # noqa: E402


class Layout(dict):
    def __init__(self, offsets, widths):
        super().__init__(offsets)
        self.widths = widths


def record():
    saved = {"frame": 9001, "screen": 2, "units": 40, "buildings": 20, "fingerprint": "a" * 64}
    reference = {"frame": 9901, "screen": 1, "units": 44, "buildings": 21, "fingerprint": "b" * 64}
    return {"saved": saved, "reference": reference, "restored": dict(saved, screen=1),
            "resumed": dict(reference), "sram_bytes": 32768, "sram_unchanged": True,
            "loaded": True, "continue": 900}


class Verdict(unittest.TestCase):
    def test_valid(self):
        self.assertTrue(sram.verdict(record())["ok"])

    def test_each_failure_fails_closed(self):
        cases = {
            "restored_matches_saved": lambda r: r["restored"].update(fingerprint="c" * 64),
            "continuation_matches_reference": lambda r: r["resumed"].update(fingerprint="c" * 64),
            "loaded": lambda r: r.update(loaded=False),
            "sram_size": lambda r: r.update(sram_bytes=8192),
            "sram_unchanged_by_load": lambda r: r.update(sram_unchanged=False),
            "saved_in_pause": lambda r: r["saved"].update(screen=1),
            "continued": lambda r: r["reference"].update(frame=9001),
            "world_has_opponents": lambda r: r["saved"].update(buildings=0),
        }
        for check, mutate in cases.items():
            r = copy.deepcopy(record())
            mutate(r)
            if check == "continued":
                r["resumed"]["frame"] = 9001
            result = sram.verdict(r)
            self.assertFalse(result["ok"], check)
            self.assertFalse(result["checks"][check], check)

    def test_frame_mismatch_with_equal_fingerprint_fails(self):
        r = record()
        r["restored"]["frame"] = 9002
        self.assertFalse(sram.verdict(r)["checks"]["restored_matches_saved"])

    def test_missing_or_malformed_phase_is_an_error(self):
        r = record()
        del r["restored"]
        with self.assertRaisesRegex(sram.ContinuationError, "lacks"):
            sram.verdict(r)
        r = record()
        r["resumed"]["fingerprint"] = "short"
        with self.assertRaisesRegex(sram.ContinuationError, "malformed"):
            sram.verdict(r)


class EntityRows(unittest.TestCase):
    def test_reads_live_rows_at_dwarf_widths(self):
        layout = Layout({"alive": 0, "owner": 1, "x": 2, "hp": 4},
                        {"alive": 1, "owner": 1, "x": 2, "hp": 4})
        live = bytes([1, 3]) + (-5).to_bytes(2, "little", signed=True) + (40).to_bytes(4, "little")
        dead = bytes(8)
        self.assertEqual(sram.entity_rows(dead + live, 8, layout, ("owner", "x", "hp")), [(1, 3, -5, 40)])

    def test_rejects_missing_field_and_bad_stride(self):
        layout = Layout({"alive": 0, "owner": 1}, {"alive": 1, "owner": 1})
        with self.assertRaisesRegex(sram.ContinuationError, "lacks scalar field hp"):
            sram.entity_rows(bytes(8), 8, layout, ("owner", "hp"))
        with self.assertRaisesRegex(sram.ContinuationError, "stride"):
            sram.entity_rows(bytes(7), 8, layout, ("owner",))

    def test_fingerprint_is_order_independent_for_keys(self):
        self.assertEqual(sram.fingerprint({"a": 1, "b": [1, 2]}), sram.fingerprint({"b": [1, 2], "a": 1}))
        self.assertNotEqual(sram.fingerprint({"a": 1}), sram.fingerprint({"a": 2}))


if __name__ == "__main__":
    unittest.main()
