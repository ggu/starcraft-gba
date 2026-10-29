"""Asset-free checks for the T9 normal-menu ARM replay tape and comparison."""
from __future__ import annotations
import importlib.util
import json
from pathlib import Path
import re
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "sc64_t9_arm_replay", ROOT / "tools/sc64_t9_arm_replay.py")
assert SPEC and SPEC.loader
replay = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(replay)


def observation(kind, tick, **fields):
    row = {"kind": kind, "host_tick": tick, "frame": tick, "screen": 1, "won": 0, "lost": 0,
           "mission": 12, "faults": 0, "phase": 1, "objective_flags": 0, "race_completed": 0,
           "protoss_force2_buildings": 43, "zerg_force3_buildings": 21, "raw16_alive": 1,
           "side_units": 11, "zerg_units": 43, "minerals": 0, "gas": 0, "selection": 0,
           "selected_building": -1}
    row.update(fields)
    return row


def sample(final="won", stop=2000, inputs=((2, 256), (5, 0), (9, 1), (10, 0))):
    rows = [observation("start", 0)]
    rows += [{"kind": "input", "host_tick": t, "held": h,
              "pressed": h & ~(inputs[i - 1][1] if i else 0)} for i, (t, h) in enumerate(inputs)]
    rows.append(observation("periodic", 1000, minerals=50))
    if final == "won":
        rows.append(observation("won", stop, screen=3, won=1, phase=3, objective_flags=1,
                                race_completed=4096, protoss_force2_buildings=0))
    else:
        rows.append(observation(final, stop))
    return {"trace_schema": 1, "entry": "t9-probe-entry", "profile": "terran-t9",
            "policy": "test", "mission": 12, "tick_limit": 500000, "events": rows}


def arm_log(rows):
    """Render observations as the probe's two-line debug output."""
    first = ("frame", "screen", "won", "lost", "mission", "faults", "phase",
             "objective_flags", "race_completed")
    out = ["[WARN] GBA Debug: T9 LAUNCH ok frames=14 frame=1 screen=1 mission=12 race_mode=0"]
    for row in rows:
        a = " ".join(f"{k}={row[k]}" for k in first)
        b = " ".join(f"{k}={row[k]}" for k in replay.COMPARED if k not in first)
        out.append(f"[WARN] GBA Debug: T9 CHECK {row['kind']} host_tick={row['host_tick']} {a}")
        out.append(f"[WARN] GBA Debug: T9 CHECK2 host_tick={row['host_tick']} {b}")
    out.append("[WARN] GBA Debug: T9 CYCLES frames=1999 total=5617920000 update=1000 peak=9 "
               "buckets=0,0,0,0,0,0,0,0")
    out.append("[WARN] GBA Debug: T9 DONE cursor=4")
    return "\n".join(out)


class TapeTests(unittest.TestCase):
    def write(self, trace):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        path = Path(tmp.name) / "trace.json"
        path.write_text(json.dumps(trace))
        return path, Path(tmp.name)

    def test_tape_packs_tick_and_held(self):
        path, tmp = self.write(sample())
        header = tmp / "tape.h"
        plan = replay.write_tape(path, header)
        text = header.read_text()
        self.assertEqual(plan["stop"], 2000)
        self.assertIn("#define T9_TAPE_COUNT 4u", text)
        self.assertIn("#define T9_TAPE_STOP_TICK 2000u", text)
        self.assertIn("#define T9_TAPE_OUTCOME 1", text)
        words = [int(w, 16) for w in re.findall(r"0x([0-9a-f]{8})u", text)]
        self.assertEqual([(w >> 10, w & 1023) for w in words], [(2, 256), (5, 0), (9, 1), (10, 0)])

    def test_tick_one_must_stay_neutral(self):
        path, _ = self.write(sample(inputs=((1, 256), (5, 0))))
        with self.assertRaisesRegex(ValueError, "tick 1 must be neutral"):
            replay.load_trace(path)

    def test_rejects_inconsistent_pressed_mask(self):
        trace = sample()
        trace["events"][2]["pressed"] = 256
        path, _ = self.write(trace)
        with self.assertRaisesRegex(ValueError, "pressed mask"):
            replay.load_trace(path)

    def test_rejects_other_missions_and_late_inputs(self):
        trace = sample()
        trace["mission"] = 11
        path, _ = self.write(trace)
        with self.assertRaisesRegex(ValueError, "not a T9"):
            replay.load_trace(path)
        trace = sample()
        trace["events"].append({"kind": "input", "host_tick": 2000, "held": 1, "pressed": 1})
        path, _ = self.write(trace)
        with self.assertRaisesRegex(ValueError, "terminal observation must be last"):
            replay.load_trace(path)


class CompareTests(unittest.TestCase):
    def plan(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        path = Path(tmp.name) / "trace.json"
        path.write_text(json.dumps(sample()))
        return replay.load_trace(path)

    def test_matching_log_has_no_mismatches(self):
        plan = self.plan()
        rows = [plan["observations"][1000], plan["final"]]
        result = replay.compare(plan, arm_log(rows))
        self.assertEqual(result["mismatches"], [])
        self.assertEqual(result["compared_observations"], 2)
        self.assertTrue(result["done"])
        self.assertEqual(result["arm_final"]["won"], 1)
        self.assertEqual(result["cycles"]["total"], 5617920000)

    def test_field_and_missing_observations_are_reported(self):
        plan = self.plan()
        changed = dict(plan["observations"][1000], gas=7)
        result = replay.compare(plan, arm_log([changed]))
        self.assertIn("tick 1000 gas: ARM=7 host=0", result["mismatches"])
        self.assertIn("missing ARM observation at tick 2000", result["mismatches"])


if __name__ == "__main__":
    unittest.main()
