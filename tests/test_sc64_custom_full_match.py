# SPDX-License-Identifier: GPL-3.0-or-later
from __future__ import annotations

import json
from pathlib import Path
import sys
import tempfile
from types import SimpleNamespace
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import sc64_custom_full_match as match  # noqa: E402


def check(kind, tick, frame):
    return {"kind": kind, "host_tick": tick, "frame": frame, "screen": 1, "minerals": 50, "gas": 0,
            "hash": 7, "owners": [[4, 1], [4, 1], [0, 0], [4, 1]]}


def trace(save=40):
    events = [{"kind": "input", "host_tick": 1, "held": 128}, {"kind": "input", "host_tick": 2, "held": 0},
              {"kind": "start", "host_tick": 20, "frame": 1, "map": 0, "human_race": 0, "melee": 0,
               "start_order": 0, "bots": [{"owner": 1, "race": 1}]},
              {"kind": "plan", "host_tick": 20, "values": [1]},
              check("check", 25, 6)]
    # The fixture runs at a 25-update cadence; tests that load it patch CHECK_INTERVAL.
    for offset, held in ((-2, 8), (-1, 0), (0, 2), (1, 0), (2, 1), (3, 0)):
        events.append({"kind": "input", "host_tick": save + offset, "held": held})
        if offset == 0:
            events.append({"kind": "save", "host_tick": save, "frame": 18, "hash": 9, "visible_own": 20,
                           "visible_hostile": 8, "cursor": [100, 100]})
    events += [{"kind": "resume", "host_tick": save + 7, "frame": 23, "hash": 9},
               {"kind": "input", "host_tick": save + 9, "held": 16},
               check("check", 50, 26),
               {"kind": "input", "host_tick": save + 12, "held": 0},
               dict(check("end", 60, 34), screen=3)]
    return {"profile": "custom-full-match", "map": 0, "human": "t", "cpus": "z", "check_interval": 250,
            "events": events,
            "terminal": {"kind": "won", "host_tick": 60, "frame": 34, "won": 1, "lost": 0,
                         "save_tick": save, "save_frame": 18, "resume_tick": save + 7}}


def kinds(raw):
    return [row["kind"] for row in raw["events"]]


def load(raw):
    match.CHECK_INTERVAL, saved = 25, match.CHECK_INTERVAL
    raw = dict(raw, check_interval=25) if raw.get("check_interval") == 250 else raw
    try:
        return _load(raw)
    finally:
        match.CHECK_INTERVAL = saved


def _load(raw):
    with tempfile.TemporaryDirectory() as folder:
        path = Path(folder) / "trace.json"
        path.write_text(json.dumps(raw))
        return match.load_trace(path)


class Trace(unittest.TestCase):
    def test_fixture_layout(self):
        # The malformed cases below edit records by position.
        self.assertEqual([kinds(trace())[i] for i in (2, 4, 8, 12, 14, 16)],
                         ["start", "check", "save", "resume", "check", "end"])

    def test_valid_trace_is_indexed_by_completed_update(self):
        loaded = load(trace())
        self.assertEqual(loaded["inputs"][:2], [(0, 128), (1, 0)])
        self.assertEqual(sorted(loaded["checks"]), [25, 50, 60])
        self.assertEqual((loaded["save"]["host_tick"], loaded["resume"]["host_tick"], loaded["end"]), (40, 47, 60))

    def test_rejects_malformed_traces(self):
        cases = {
            "profile": lambda raw: raw.update(profile="other"),
            "terminal": lambda raw: raw.pop("terminal"),
            "opposing keys": lambda raw: raw["events"][0].update(held=48),
            "repeated update": lambda raw: raw["events"][1].update(host_tick=1),
            "order": lambda raw: raw["events"][4].update(host_tick=3),
            "no start": lambda raw: raw["events"].pop(2),
            "no end": lambda raw: raw["events"].pop(),
            "missing check": lambda raw: raw["events"].pop(14),
            "save without resume": lambda raw: raw["events"].pop(12),
            "unknown": lambda raw: raw["events"].append({"kind": "write", "host_tick": 61}),
            "check without hash": lambda raw: raw["events"][4].pop("hash"),
            "check without owners": lambda raw: raw["events"][4].pop("owners"),
            "bare check": lambda raw: raw["events"].insert(5, {"kind": "check", "host_tick": 30}),
            "conflicting check": lambda raw: raw["events"].insert(5, dict(check("train", 25, 7))),
            "save without hash": lambda raw: raw["events"][8].pop("hash"),
            "save without cursor": lambda raw: raw["events"][8].pop("cursor"),
            "resume without hash": lambda raw: raw["events"][12].pop("hash"),
            "no check interval": lambda raw: raw.pop("check_interval"),
            "declared cadence": lambda raw: raw.update(check_interval=1000),
            "no bots": lambda raw: raw["events"][2].pop("bots"),
            "contradictory terminal": lambda raw: raw["terminal"].update(lost=1),
            "terminal frame": lambda raw: raw["terminal"].update(frame=35),
            "wrong pause keys": lambda raw: raw["events"][7].update(held=1),
        }
        for name, change in cases.items():
            raw = trace()
            change(raw)
            with self.subTest(name), self.assertRaises(match.MatchError):
                load(raw)


class Matrix(unittest.TestCase):
    def args(self, cases, folder):
        return SimpleNamespace(cases=cases, output=Path(folder) / "out", runner=Path("/runner"),
                               limit=100, jobs=1)

    def test_rejects_bad_cases_before_running_anything(self):
        with tempfile.TemporaryDirectory() as folder:
            for cases in (["showcase:0:z"], ["terran:2:z"], ["terran:0:zx"], ["terran:0:"],
                          ["terran:0:zzzz"], ["terran:0:z:LINE=x"], ["terran:0:z:a:b"],
                          ["terran:0:z", "terran:0:z:PRODUCERS=3"]):
                with self.subTest(cases), self.assertRaises(match.MatchError):
                    match.matrix(self.args(cases, folder))
            self.assertFalse((Path(folder) / "out").exists())

    def test_default_matrix_covers_each_race_map_and_opponent_count(self):
        expected = {(product, map_id, count) for product in match.PRODUCTS
                    for map_id in match.MAP_NAMES for count in (1, 2, 3)}
        covered = set()
        for case in match.QUALIFIED:
            product, map_id, cpus, *plan = case.split(":")
            self.assertTrue(all(race in match.RACES for race in cpus))
            match.parse_plan(plan[0] if plan else "")
            covered.add((product, int(map_id), len(cpus)))
        self.assertEqual(covered, expected)


class Author(unittest.TestCase):
    def test_a_failed_player_cannot_reuse_an_old_trace(self):
        with tempfile.TemporaryDirectory() as folder:
            out = Path(folder)
            (out / "trace.json").write_text(json.dumps(trace()))
            fail = out / "fail.sh"
            fail.write_text("#!/bin/sh\nexit 1\n")
            fail.chmod(0o755)
            args = SimpleNamespace(output=out, map=0, plan="", controller=fail, human="t", cpus="z", limit=10)
            with self.assertRaisesRegex(match.MatchError, "no trace"):
                match.author(args)

    def test_a_terminal_only_trace_is_refused(self):
        with tempfile.TemporaryDirectory() as folder:
            out = Path(folder)
            (out / "bare.json").write_text(json.dumps({"terminal": {"kind": "won"}}))
            fake = out / "fake.sh"
            fake.write_text("#!/bin/sh\ncp '%s' \"$4\"\nexit 0\n" % (out / "bare.json"))
            fake.chmod(0o755)
            args = SimpleNamespace(output=out, map=0, plan="", controller=fake, human="t", cpus="z", limit=10)
            with self.assertRaisesRegex(match.MatchError, "not a custom-full-match trace"):
                match.author(args)

    def test_exit_status_must_agree_with_the_trace(self):
        with tempfile.TemporaryDirectory() as folder:
            out = Path(folder)
            liar = out / "liar.sh"
            # Writes a winning trace but exits as a defeat.
            liar.write_text("#!/bin/sh\ncp '%s' \"$4\"\nexit 3\n" % (out / "won.json"))
            liar.chmod(0o755)
            (out / "won.json").write_text(json.dumps(trace()))
            args = SimpleNamespace(output=out, map=0, plan="", controller=liar, human="t", cpus="z", limit=10)
            with self.assertRaisesRegex(match.MatchError, "exited 3"):
                match.author(args)

    def test_plan_settings(self):
        self.assertEqual(match.parse_plan("FIRST_ATTACK=4500,LINE=38"), {"FIRST_ATTACK": 4500, "LINE": 38})
        self.assertEqual(match.parse_plan(""), {})
        for text in ("first=1", "LINE", "LINE=a", "LINE=1;rm"):
            with self.subTest(text), self.assertRaises(match.MatchError):
                match.parse_plan(text)


class Continuation(unittest.TestCase):
    def test_tail_is_renumbered_for_a_fresh_load(self):
        loaded = load(trace())
        shift, tail = match.continuation_tail(loaded["inputs"], 40)
        # Uninterrupted update 43 releases A; the fresh process releases L at its fourth update.
        self.assertEqual((shift, tail), (39, [(3, 0), (9, 16), (12, 0)]))

    def test_rejects_a_tape_without_the_ordinary_pause(self):
        loaded = load(trace())
        for index, held in ((5, 4), (7, 512), (9, 2)):
            inputs = list(loaded["inputs"])
            inputs[index - 3] = (inputs[index - 3][0], held)
            with self.subTest(index), self.assertRaises(match.MatchError):
                match.continuation_tail(inputs, 40)

    def test_rejects_keys_before_the_resumed_world_settles(self):
        raw = trace()
        raw["events"][13]["host_tick"] = 45  # the first key after the resume
        with self.assertRaises(match.MatchError):
            match.continuation_tail(load(raw)["inputs"], 40)


class Observation(unittest.TestCase):
    def test_bitmap_counts_displayed_colors_on_the_selected_page(self):
        palette = bytearray(512)
        memory = {0x04000000: b"\x04\x04", 0x05000000: palette,
                  0x06000000: bytes(240 * 160), 0x0600A000: bytes([1]) * (240 * 160)}
        game = SimpleNamespace(rsp=SimpleNamespace(read=lambda address, size: bytes(memory[address][:size])))
        black = match.displayed_bitmap(game)
        # Page two uses index one. Both indices initially resolve to black.
        memory[0x04000000] = b"\x14\x04"
        palette[4] = 31
        palette[3] = 128
        self.assertEqual(black, match.displayed_bitmap(game))
        palette[2] = 31
        self.assertNotEqual(black, match.displayed_bitmap(game))
        for control in (b"\x84\x04", b"\x04\x00"):
            memory[0x04000000] = control
            with self.assertRaises(match.MatchError):
                match.displayed_bitmap(game)

    def test_world_hash_matches_the_author(self):
        # FNV-1a over 32-bit words, as world_hash() in tests/sc64_custom_full_match.c.
        state = {"units": [(3, 0, 1, 100, -2, 40, 9, 9)], "buildings": [(0, 4, 2, 64, 72, 500, 1)],
                 "minerals": 50, "gas": 0, "frame": 12}
        expected = 2166136261
        for value in (3, 0, 1, 100, -2, 40, 0, 4, 2, 64, 72, 500, 50, 0, 12):
            expected = ((expected ^ (value & 0xFFFFFFFF)) * 16777619) & 0xFFFFFFFF
        self.assertEqual(match.world_hash(state), expected)
        self.assertNotEqual(match.world_hash(dict(state, frame=13)), expected)

    def test_compare_names_each_difference(self):
        expected = check("check", 25, 6)
        actual = dict(expected, frame=7, owners=[[4, 1], [3, 1], [0, 0], [4, 1]])
        self.assertEqual(match.compare(expected, expected), [])
        self.assertEqual([line.split("=")[0] for line in match.compare(expected, actual)], ["frame", "owners"])


if __name__ == "__main__":
    unittest.main()
