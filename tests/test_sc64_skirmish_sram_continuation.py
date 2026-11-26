# SPDX-License-Identifier: GPL-3.0-or-later
from __future__ import annotations

import copy
from pathlib import Path
import sys
from types import SimpleNamespace
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import sc64_skirmish_sram_continuation as sram  # noqa: E402


class Layout(dict):
    def __init__(self, offsets, widths, arrays=None):
        super().__init__(offsets)
        self.widths = widths
        self.arrays = arrays or {}


def record():
    saved = {"frame": 9001, "screen": 2, "units": 40, "buildings": 20, "fingerprint": "a" * 64}
    reference = {"frame": 9901, "screen": 1, "units": 44, "buildings": 21, "fingerprint": "b" * 64}
    return {"saved": saved, "reference": reference, "restored": dict(saved, screen=1),
            "resumed": dict(reference), "sram_bytes": 32768, "sram_unchanged": True,
            "loaded": True, "continue": 900}


class Verdict(unittest.TestCase):
    def test_physical_rejects_tampering_before_accessing_output(self):
        with self.assertRaisesRegex(sram.ContinuationError, "cannot tamper"):
            sram.run(SimpleNamespace(physical=True, tamper_sram=True))

    def test_valid(self):
        self.assertTrue(sram.verdict(record())["ok"])

    def test_rechecking_physical_receipt_rejects_changed_cpu_seats(self):
        r = record()
        lobby = {"race_mode": 1, "race_human": 2, "race_bots": 1, "race_skirmish_map": 1}
        seats = {"mask": 4, "races": "t", "base_ids": [0, 3], "start_order": 0}
        r.update(transport="physical-buttons", human="p", bots="t", map=1,
                 seats="--t", melee=True, saved_game_type=1, restored_game_type=1,
                 saved_lobby=lobby, restored_lobby=lobby, launch_seats=seats,
                 saved_seats=seats, restored_seats=seats)
        self.assertTrue(sram.verdict(r)["ok"])
        r["restored_seats"] = dict(seats, mask=1)
        self.assertFalse(sram.verdict(r)["ok"])

    def test_opening_rejects_wrong_faction_units_and_hall(self):
        buildings = [(0, 0, 13, 10, 10), (1, 1, 6, 20, 20)]
        units = [(i, 0, 9) for i in range(4)] + [(i + 4, 1, 7) for i in range(4)] + [(8, 1, 8)]
        game = SimpleNamespace(state=lambda: {"buildings": buildings, "units": units},
                               u32=lambda name, offset=0: 2 if name == "race_human" else 10 + 10 * (offset // 4))
        seats = {"mask": 1, "races": "z", "base_ids": [0, 1]}
        sram.validate_launch_seats(game, "z--", seats)
        units[0] = (0, 0, 0)
        with self.assertRaisesRegex(sram.ContinuationError, "opening units"):
            sram.validate_launch_seats(game, "z--", seats)
        buildings[0] = (0, 0, 0, 10, 10)
        with self.assertRaisesRegex(sram.ContinuationError, "opening halls"):
            sram.validate_launch_seats(game, "z--", seats)

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


class CompletedUpdates(unittest.TestCase):
    def game(self, stops, initial=40):
        class Replay:
            def __init__(self):
                self.current = initial
                self.stops = iter(stops)
                self.calls = 0
                self.writes = []

            def tick(self, *, expected):
                self.calls += 1
                self.current = next(self.stops)

            def write(self, address, data):
                self.writes.append((address, data))

        game = sram.Game.__new__(sram.Game)
        game.rsp = Replay()
        game.u32 = lambda name: game.rsp.current
        game.sym = {"demo_schedule_events": 1, "demo_schedule_count": 2, "demo_schedule_index": 3}
        game.update, game.keys, game.updates, game.stale_update_stops = 4, 0, 0, 0
        return game

    def test_reentered_stop_is_not_another_update(self):
        game = self.game([40, 41, 42])
        game.step(0, 2)
        self.assertEqual((game.updates, game.rsp.calls, game.stale_update_stops), (2, 3, 1))

    def test_reentered_stop_does_not_resend_input(self):
        game = self.game([40, 41])
        game.step(sram.KEY_A)
        self.assertEqual(len(game.rsp.writes), 3)
        self.assertEqual(game.rsp.writes[0], (1, (41).to_bytes(4, "little") + (1).to_bytes(4, "little")))

    def test_skipped_or_backwards_ticks_fail(self):
        for tick in (39, 42):
            game = self.game([tick])
            with self.assertRaisesRegex(sram.ContinuationError, "scheduler jumped"):
                game.step()
            self.assertEqual(game.updates, 0)

    def test_stationary_scheduler_fails_with_a_bound(self):
        game = self.game([40] * 8)
        with self.assertRaisesRegex(sram.ContinuationError, "eight breakpoint stops"):
            game.step()
        self.assertEqual((game.updates, game.rsp.calls), (0, 8))

    def test_window_boundary_still_counts_one_update(self):
        game = self.game([250, 251], initial=249)
        game.step(0, 2)
        self.assertEqual(game.updates, 2)
        self.assertEqual(game.stale_update_stops, 0)

    def test_bootstrap_is_separate_from_completed_update_counts(self):
        game = self.game([0, 1, 2], initial=0)
        game.synchronize_scheduler()
        self.assertEqual((game.bootstrap_stops, game.updates, game.stale_update_stops), (2, 0, 0))
        game.step(0)
        self.assertEqual((game.updates, game.stale_update_stops), (1, 0))


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

    def test_active_queue_detects_changes_and_ignores_inactive_ring_cells(self):
        for width in (1, 4):
            with self.subTest(width=width):
                stride = 4 + 5 * width
                layout = Layout({"alive": 0, "queue_head": 1, "queue_count": 2, "queue": 4},
                                {"alive": 1, "queue_head": 1, "queue_count": 1},
                                {"queue": (5, width, width == 4)})
                row = bytearray(stride); row[0:3] = bytes((1, 4, 2))
                def put(slot, value):
                    row[4 + slot * width:4 + (slot + 1) * width] = value.to_bytes(width, "little")
                put(4, 7); put(0, 11)
                def digest():
                    return sram.fingerprint({"buildings": sram.entity_rows(
                        bytes(row), stride, layout, ("queue_head", "queue_count"), active_queue=True)})
                before = digest(); put(2, 17)
                self.assertEqual(before, digest(), "inactive slot must not affect state")
                put(0, 12)
                self.assertNotEqual(before, digest(), "active queued type must affect state")

    def test_queue_layout_and_state_fail_closed(self):
        fields = ("queue_head", "queue_count")
        for metadata, offset in ((None, 4), ((4, 1, False), 4), ((5, 3, False), 4),
                                 ((5, 1, False), -1), ((5, 1, False), 5)):
            layout = Layout({"alive": 0, "queue_head": 1, "queue_count": 2, "queue": offset},
                            {"alive": 1, "queue_head": 1, "queue_count": 1},
                            {"queue": metadata} if metadata else {})
            with self.subTest(metadata=metadata, offset=offset), self.assertRaises(sram.ContinuationError):
                sram.entity_rows(bytes([1, 0, 1]) + bytes(6), 9, layout, fields, active_queue=True)
        layout = Layout({"alive": 0, "queue_head": 1, "queue_count": 2, "queue": 4},
                        {"alive": 1, "queue_head": 1, "queue_count": 1}, {"queue": (5, 1, False)})
        for head, count in ((5, 1), (0, 6)):
            with self.assertRaisesRegex(sram.ContinuationError, "queue count or head"):
                sram.entity_rows(bytes([1, head, count]) + bytes(6), 9, layout, fields, active_queue=True)

    def test_fingerprint_is_order_independent_for_keys(self):
        self.assertEqual(sram.fingerprint({"a": 1, "b": [1, 2]}), sram.fingerprint({"b": [1, 2], "a": 1}))
        self.assertNotEqual(sram.fingerprint({"a": 1}), sram.fingerprint({"a": 2}))


if __name__ == "__main__":
    unittest.main()
