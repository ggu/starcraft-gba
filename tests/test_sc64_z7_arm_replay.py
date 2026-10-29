import json
import struct
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

import sc64_z7_arm_replay as replay

TAG, PAYLOAD = replay.SAVE_TAG, replay.SAVE_PAYLOAD_BYTES


def observation(kind, tick, **changes):
    row = {"kind": kind, "host_tick": tick, "frame": tick - 11, "screen": 1, "won": 0,
           "lost": 0, "outcome": 0, "started": 2, "fired": 2,
           "action_cursors": [0, 1, 0, 0, 0, 0, 0, 0],
           "action_ticks": [0, 1, 0, 0, 0, 0, 0, 0], "switch10": 0,
           "ai_script_mask": 0, "ai_plan_ticks": 0, "ai_plan_phase": 0, "fault": 0,
           "side_units": 20, "side_buildings": 1, "enemy_units": 93,
           "enemy_buildings": 57, "workers": 4, "minerals": 800, "gas": 0,
           "action_count": 7}
    row.update(changes)
    return row


def trace(kind="won"):
    final = {"won": dict(screen=3, won=1, outcome=1, started=254, enemy_buildings=0),
             "limit": {}}[kind]
    return {"schema": 1, "profile": "zerg-z7", "mission": 18, "policy": "fixture", "events": [
        {"kind": "input", "host_tick": 1, "held": 1, "pressed": 1},
        {"kind": "input", "host_tick": 2, "held": 0, "pressed": 0},
        observation("start", 12),
        {"kind": "input", "host_tick": 20, "held": 16, "pressed": 16},
        observation("checkpoint", 300, minerals=900),
        {"kind": "input", "host_tick": 301, "held": 0, "pressed": 0},
        observation(kind, 600, **final),
    ]}


def replay_lines(row, kind="checkpoint"):
    scalars = " ".join(f"{f}={row[f]}" for f in replay.SCALARS[:11])
    lists = " ".join(f"{f}={','.join(map(str, row[f]))}" for f in replay.LISTS)
    rest = " ".join(f"{f}={row[f]}" for f in replay.SCALARS[11:])
    tick = row["host_tick"]
    return [f"[WARN] GBA Debug: Z7 CHECK {kind} host_tick={tick} {scalars}",
            f"[WARN] GBA Debug: Z7 CHECK2 host_tick={tick} {lists}",
            f"[WARN] GBA Debug: Z7 CHECK3 host_tick={tick} {rest}"]


def cycles(kind, tick, frames, total=None):
    total = frames * 1000 if total is None else total
    return (f"Z7 CYCLES {kind} host_tick={tick} frames={frames} total={total} "
            f"update={total // 2} peak=2000 update_peak=1500 stack_painted=11776 "
            "stack_untouched=8000")


def run_log(rows, *, terminal_kind="won", start=True, done=True, timing=("terminal", 600, 600),
            head=(), extra=()):
    lines = ["Z7 START mode=0 tape=4 checks=3 stop=600"] if start else []
    lines += list(head)
    for row in rows:
        lines += replay_lines(row)
    if terminal_kind:
        lines += replay_lines(rows[-1], terminal_kind)
    if timing:
        lines.append(cycles(*timing))
    lines += list(extra)
    if done:
        lines.append("Z7 DONE cursor=4")
    return "\n".join(lines) + "\n"


def sram(*records):
    """32 KiB alternating-slot SRAM with (slot, sequence, payload, valid) rows."""
    data = bytearray(b"\xff" * replay.SAVE_BYTES)
    for slot, sequence, payload, valid in records:
        h = 2166136261
        for byte in payload:
            h = ((h ^ byte) * 16777619) & 0xFFFFFFFF
        off = slot * replay.SAVE_SLOT_SIZE
        struct.pack_into("<4I", data, off, TAG, len(payload), h if valid else h ^ 1, sequence)
        data[off + 16:off + 16 + len(payload)] = payload
    return bytes(data)


PAUSE = bytes([1]) * PAYLOAD
WIN = bytes([2]) * PAYLOAD


class Z7ArmReplayTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)

    def plan(self, kind="won"):
        path = Path(self.tmp.name) / "trace.json"
        path.write_text(json.dumps(trace(kind)))
        return replay.load_trace(path)

    def rows(self, plan):
        return [plan["observations"][t] for t in sorted(plan["observations"])]

    def full(self, plan, text, data=None):
        return replay.evaluate(plan, "full", text, None,
                               sram=sram((0, 1, WIN, True)) if data is None else data)

    def assertFails(self, result, needle):
        self.assertFalse(result["qualified"])
        self.assertTrue(any(needle in f for f in result["failures"]), result["failures"])

    def test_tape_header_encodes_transitions_and_check_ticks(self):
        path = Path(self.tmp.name) / "trace.json"
        path.write_text(json.dumps(trace()))
        out = Path(self.tmp.name) / "tape.h"
        plan = replay.write_tape(path, out)
        text = out.read_text()
        self.assertEqual(plan["stop"], 600)
        self.assertIn("#define Z7_TAPE_COUNT 4u", text)
        self.assertIn("#define Z7_CHECK_COUNT 3u", text)
        self.assertIn(f"0x{(20 << 10) | 16:08x}u", text)
        self.assertIn("0x0000000cu,0x0000012cu,0x00000258u", text)

    def test_rejects_inconsistent_or_unterminated_traces(self):
        path = Path(self.tmp.name) / "bad.json"
        for edit, needle in ((lambda t: t["events"][3].update(pressed=0), "pressed mask"),
                             (lambda t: t["events"].pop(), "terminal"),
                             (lambda t: t.update(profile="zerg-z6"), "zerg-z7")):
            bad = trace()
            edit(bad)
            path.write_text(json.dumps(bad))
            with self.assertRaisesRegex(ValueError, needle):
                replay.load_trace(path)

    def test_full_replay_qualifies_with_exact_match_timing_and_sram(self):
        plan = self.plan()
        result = self.full(plan, run_log(self.rows(plan)))
        self.assertTrue(result["qualified"], result["failures"])
        self.assertEqual(result["compared_observations"], 3)
        self.assertEqual(result["guest_timing"]["frames_per_second"],
                         round(600 * replay.CPU_HZ / 600000, 3))
        self.assertEqual([(r["slot"], r["sequence"]) for r in result["sram_records"]], [(0, 1)])

    def test_mismatch_and_missing_terminal_fail(self):
        plan = self.plan()
        rows = self.rows(plan)
        self.assertFails(self.full(plan, run_log([rows[0], dict(rows[1], gas=1), rows[2]])),
                         "tick 300 gas: replay=1 host=0")
        self.assertFails(self.full(plan, run_log(rows, terminal_kind=None)), "terminal observation")
        self.assertFails(self.full(plan, run_log(rows, done=False)), "one DONE")
        self.assertFails(self.full(plan, run_log(rows, start=False)), "one START")
        self.assertFails(self.full(plan, run_log(rows[:2])), "compared 2 of 3")

    def test_timing_gate_fails_closed(self):
        plan = self.plan()
        rows = self.rows(plan)
        self.assertFails(self.full(plan, run_log(rows, timing=None)),
                         "expected one terminal timing record, found 0")
        self.assertFails(self.full(plan, run_log(rows, extra=[cycles("terminal", 600, 600)])),
                         "found 2")
        self.assertFails(self.full(plan, run_log(rows, timing=("terminal", 600, 599))),
                         "expected 600")
        self.assertFails(self.full(plan, run_log(rows, timing=("terminal", 600, 600, 10 ** 9))),
                         "incoherent timing")
        self.assertFails(self.full(plan, run_log(rows, extra=[cycles("periodic", 300, 700)])),
                         "incoherent timing")
        untimed = replay.evaluate(plan, "full", run_log(rows, timing=None), None, timed=False,
                                  sram=sram((0, 1, WIN, True)))
        self.assertTrue(untimed["qualified"], untimed["failures"])
        self.assertIsNone(untimed["guest_timing"])

    def test_malformed_unknown_and_duplicate_records_fail(self):
        plan = self.plan()
        rows = self.rows(plan)
        lists_short = "Z7 CHECK2 host_tick=300 action_cursors=0,1 action_ticks=0,1,0,0,0,0,0,0"
        self.assertFails(self.full(plan, run_log(rows, extra=[lists_short])), "malformed")
        self.assertFails(self.full(plan, run_log(rows, extra=["Z7 BOGUS host_tick=1"])), "unknown record")
        self.assertFails(self.full(plan, run_log(rows, extra=["Z7 CHECK3 host_tick=300 gas=5"])),
                         "malformed")
        conflicting = replay_lines(dict(rows[1], minerals=1))[2]
        self.assertFails(self.full(plan, run_log(rows, extra=[conflicting])), "conflicting minerals")
        self.assertFails(self.full(plan, run_log(rows, extra=replay_lines(rows[1]))),
                         "duplicate observation kind")
        repeated = replay_lines(rows[1])[0] + " frame=1"
        self.assertFails(self.full(plan, run_log(rows, extra=[repeated])), "repeats")
        self.assertFails(self.full(plan, run_log(rows, extra=replay_lines(dict(rows[1], host_tick=301)))),
                         "unexpected replay observation at tick 301")

    def test_sram_gate_fails_closed(self):
        plan = self.plan()
        log = run_log(self.rows(plan))
        self.assertFails(replay.evaluate(plan, "full", log, None), "SRAM not verified")
        self.assertFails(self.full(plan, log, data=b"\0" * 10), "SRAM is 10 bytes")
        self.assertFails(self.full(plan, log, data=sram((0, 1, WIN, False))), "invalid SRAM record")
        self.assertFails(self.full(plan, log, data=sram()), "do not match its outcome")
        self.assertFails(self.full(plan, log, data=sram((0, 1, b"x" * 10, True))),
                         "invalid SRAM record")

    def test_limit_trace_needs_no_win_or_autosave(self):
        plan = self.plan("limit")
        result = replay.evaluate(plan, "full", run_log(self.rows(plan), terminal_kind="limit"),
                                 None, sram=sram())
        self.assertTrue(result["qualified"], result["failures"])

    def prefix_log(self, plan, cut=301, saved=True):
        rows = self.rows(plan)
        cut_row = dict(rows[1], host_tick=cut)
        extra = [f"Z7 SAVED ok cut={cut} frame=290 payload_bytes={PAYLOAD} tag={TAG}"] if saved else []
        lines = (["Z7 START mode=1 tape=4 checks=3 stop=600"] + replay_lines(rows[0]) +
                 replay_lines(rows[1]) + replay_lines(cut_row, "cut") +
                 [cycles("periodic", 300, 300), cycles("cut", cut, cut)] + extra)
        return "\n".join(lines) + "\n"

    def test_prefix_requires_an_in_range_cut_and_one_pause_save(self):
        plan = self.plan()
        good = replay.evaluate(plan, "prefix", self.prefix_log(plan), None,
                               sram=sram((0, 1, PAUSE, True)))
        self.assertTrue(good["qualified"], good["failures"])
        self.assertEqual((good["cut"], good["compared_observations"]), (301, 2))
        only_saved = replay.evaluate(plan, "prefix", "Z7 SAVED ok\n", None,
                                     sram=sram((0, 1, PAUSE, True)))
        self.assertFails(only_saved, "prefix cut None is not inside the tape")
        self.assertFails(only_saved, "compared 0 of 0")
        self.assertFails(replay.evaluate(plan, "prefix", self.prefix_log(plan, saved=False), None,
                                         sram=sram((0, 1, PAUSE, True))), "one successful SAVED")
        self.assertFails(replay.evaluate(plan, "prefix", self.prefix_log(plan, cut=700), None,
                                         sram=sram((0, 1, PAUSE, True))), "not inside the tape")
        self.assertFails(replay.evaluate(plan, "prefix", self.prefix_log(plan), None,
                                         sram=sram((0, 1, PAUSE, True), (1, 2, WIN, True))),
                         "exactly one pause save")
        doubled = self.prefix_log(plan) + f"Z7 SAVED ok cut=301 frame=290 payload_bytes={PAYLOAD} tag={TAG}\n"
        self.assertFails(replay.evaluate(plan, "prefix", doubled, None,
                                         sram=sram((0, 1, PAUSE, True))), "one successful SAVED")

    def test_resume_needs_the_seed_and_a_new_victory_record(self):
        plan = self.plan()
        rows = self.rows(plan)
        seed = sram((0, 1, PAUSE, True))
        log = run_log(rows[2:], head=["Z7 LOADED ok frame=290 screen=1"],
                      timing=("terminal", 600, 600 - 301 + 1))
        good = replay.evaluate(plan, "resume", log, 301, seed=seed,
                               sram=sram((0, 1, PAUSE, True), (1, 2, WIN, True)))
        self.assertTrue(good["qualified"], good["failures"])
        self.assertEqual(good["compared_observations"], 1)
        self.assertFails(replay.evaluate(plan, "resume", log, 301,
                                         sram=sram((0, 1, PAUSE, True), (1, 2, WIN, True))),
                         "seed not supplied")
        self.assertFails(replay.evaluate(plan, "resume", log, 301, seed=seed,
                                         sram=sram((0, 1, WIN, True), (1, 2, WIN, True))),
                         "did not keep the seeded pause save")
        self.assertFails(replay.evaluate(plan, "resume", log, 301, seed=seed, sram=seed),
                         "do not match its outcome")
        self.assertFails(replay.evaluate(plan, "resume", log.replace("LOADED ok", "LOADED fail"), 301,
                                         seed=seed, sram=sram((0, 1, PAUSE, True), (1, 2, WIN, True))),
                         "one successful LOADED")
        self.assertFails(replay.evaluate(plan, "resume", log, None, seed=seed,
                                         sram=sram((0, 1, PAUSE, True), (1, 2, WIN, True))),
                         "not inside the tape")
        early = run_log(rows, head=["Z7 LOADED ok frame=290 screen=1"],
                        timing=("terminal", 600, 300))
        self.assertFails(replay.evaluate(plan, "resume", early, 301, seed=seed,
                                         sram=sram((0, 1, PAUSE, True), (1, 2, WIN, True))),
                         "unexpected replay observation at tick 12")

    def test_save_records_verify_alternating_slot_checksums(self):
        records = replay.save_records(sram((0, 1, b"\x01" * 32, True), (1, 2, b"\x01" * 32, False)))
        self.assertEqual([(r["slot"], r["sequence"], r["checksum_valid"]) for r in records],
                         [(0, 1, True), (1, 2, False)])


if __name__ == "__main__":
    unittest.main()
