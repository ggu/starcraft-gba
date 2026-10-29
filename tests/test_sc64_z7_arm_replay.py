import json
import struct
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

import sc64_z7_arm_replay as replay


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


def full_log(rows, *, done=True):
    lines = ["Z7 START mode=0 tape=4 checks=3 stop=600"]
    for row in rows:
        lines += replay_lines(row)
    lines.append("Z7 CYCLES terminal host_tick=600 frames=600 total=600000 update=500000 "
                 "peak=2000 update_peak=1500 stack_painted=11776 stack_untouched=8000")
    if done:
        lines.append("Z7 DONE cursor=4")
    return "\n".join(lines) + "\n"


class Z7ArmReplayTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)

    def write(self, data, name="trace.json"):
        path = Path(self.tmp.name) / name
        path.write_text(json.dumps(data))
        return path

    def test_tape_header_encodes_transitions_and_check_ticks(self):
        path = self.write(trace())
        out = Path(self.tmp.name) / "tape.h"
        plan = replay.write_tape(path, out)
        text = out.read_text()
        self.assertEqual(plan["stop"], 600)
        self.assertIn("#define Z7_TAPE_COUNT 4u", text)
        self.assertIn("#define Z7_CHECK_COUNT 3u", text)
        self.assertIn(f"0x{(20 << 10) | 16:08x}u", text)
        self.assertIn("0x0000000cu,0x0000012cu,0x00000258u", text)

    def test_rejects_inconsistent_or_unterminated_traces(self):
        bad = trace()
        bad["events"][3]["pressed"] = 0
        with self.assertRaisesRegex(ValueError, "pressed mask"):
            replay.load_trace(self.write(bad))
        bad = trace()
        bad["events"].pop()
        with self.assertRaisesRegex(ValueError, "terminal"):
            replay.load_trace(self.write(bad))
        bad = trace()
        bad["profile"] = "zerg-z6"
        with self.assertRaisesRegex(ValueError, "zerg-z7"):
            replay.load_trace(self.write(bad))

    def test_full_replay_qualifies_only_on_exact_match_and_win(self):
        plan = replay.load_trace(self.write(trace()))
        rows = [plan["observations"][t] for t in sorted(plan["observations"])]
        result = replay.evaluate(plan, "full", full_log(rows), None)
        self.assertTrue(result["qualified"], result["mismatches"])
        self.assertEqual(result["compared_observations"], 3)
        self.assertEqual(result["guest_timing"]["frames_per_second"], round(600 * replay.CPU_HZ / 600000, 3))
        drifted = [dict(rows[0]), dict(rows[1], gas=1), dict(rows[2])]
        result = replay.evaluate(plan, "full", full_log(drifted), None)
        self.assertFalse(result["qualified"])
        self.assertEqual(result["mismatches"], ["tick 300 gas: replay=1 host=0"])
        self.assertFalse(replay.evaluate(plan, "full", full_log(rows, done=False), None)["qualified"])

    def test_limit_trace_does_not_require_a_win(self):
        plan = replay.load_trace(self.write(trace("limit")))
        rows = [plan["observations"][t] for t in sorted(plan["observations"])]
        self.assertTrue(replay.evaluate(plan, "full", full_log(rows), None)["qualified"])

    def test_prefix_and_resume_split_checks_at_the_cut(self):
        plan = replay.load_trace(self.write(trace()))
        rows = [plan["observations"][t] for t in sorted(plan["observations"])]
        cut = dict(rows[1], host_tick=301)
        prefix = "\n".join(replay_lines(rows[0]) + replay_lines(rows[1]) + replay_lines(cut, "cut") +
                           ["Z7 SAVED ok cut=301 frame=290 payload_bytes=16296 tag=1396922964"])
        result = replay.evaluate(plan, "prefix", prefix, None)
        self.assertTrue(result["qualified"], result["mismatches"])
        self.assertEqual((result["cut"], result["compared_observations"]), (301, 2))
        resume = "Z7 LOADED ok frame=290 screen=1\n" + full_log(rows[2:])
        result = replay.evaluate(plan, "resume", resume, 301)
        self.assertTrue(result["qualified"], result["mismatches"])
        self.assertEqual(result["compared_observations"], 1)
        failed = resume.replace("LOADED ok", "LOADED fail")
        self.assertFalse(replay.evaluate(plan, "resume", failed, 301)["qualified"])

    def test_save_records_verify_alternating_slot_checksums(self):
        data = bytearray(b"\xff" * replay.SAVE_BYTES)
        payload = bytes(range(32))
        h = 2166136261
        for byte in payload:
            h = ((h ^ byte) * 16777619) & 0xFFFFFFFF
        struct.pack_into("<4I", data, 0, 0x53435A54, len(payload), h, 1)
        data[16:16 + len(payload)] = payload
        struct.pack_into("<4I", data, replay.SAVE_SLOT_SIZE, 0x53435A54, len(payload), h ^ 1, 2)
        data[replay.SAVE_SLOT_SIZE + 16:replay.SAVE_SLOT_SIZE + 16 + len(payload)] = payload
        records = replay.save_records(bytes(data))
        self.assertEqual([(r["slot"], r["sequence"], r["checksum_valid"]) for r in records],
                         [(0, 1, True), (1, 2, False)])


if __name__ == "__main__":
    unittest.main()
