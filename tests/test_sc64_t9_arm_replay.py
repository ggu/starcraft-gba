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
    end = {}
    if final == "won":
        end = dict(screen=3, won=1, phase=3, objective_flags=1, race_completed=4096,
                   protoss_force2_buildings=0)
    # The harness records a periodic checkpoint every 1000 ticks through the
    # stop tick, then the terminal one.
    for tick in range(1000, stop + 1, 1000):
        rows.append(observation("periodic", tick, **(end if tick == stop else {"minerals": 50})))
    rows.append(observation(final, stop, **end))
    return {"trace_schema": 1, "entry": "t9-probe-entry", "profile": "terran-t9",
            "policy": "test", "mission": 12, "tick_limit": 500000, "events": rows}


MENU_TIMING = "T9 MENU_CYCLES frames=13 total=3651648"
MISSION_TIMING = ("T9 CYCLES frames=2000 total=561792000 update=200000000 peak=600000 "
                  "buckets=0,1990,10,0,0,0,0,0")
START = "T9 START debug=1 tape=4 stop=2000"
MENU = tuple(f"T9 MENU key={k} screen={int(i == 6)} page={2 + (i >= 5)} row=0 "
             f"mission={12 if i >= 5 else 4}" for i, k in enumerate(replay.MENU_ROUTE))
LAUNCH = "T9 LAUNCH ok menu_frames=13 mission_frames=1 frame=1 screen=1 mission=12 race_mode=0"
DONE = "T9 DONE cursor=4"


def probe_rows(plan):
    """The observations the probe reports: launch, every 1000 ticks, final."""
    rows = [observation("launch", 1)]
    rows += [dict(plan["observations"][t], kind="periodic")
             for t in range(1000, plan["stop"] + 1, 1000)]
    return rows + [plan["final"]]


def check_lines(row):
    first = ("frame", "screen", "won", "lost", "mission", "faults", "phase",
             "objective_flags", "race_completed")
    a = " ".join(f"{k}={row[k]}" for k in first)
    b = " ".join(f"{k}={row[k]}" for k in replay.COMPARED if k not in first)
    return [f"T9 CHECK {row['kind']} host_tick={row['host_tick']} {a}",
            f"T9 CHECK2 host_tick={row['host_tick']} {b}"]


def arm_log(rows, timing=(MENU_TIMING, MISSION_TIMING), head=None, done=(DONE,)):
    """Render observations as the probe's debug output, in its line order."""
    body = [line for row in rows for line in check_lines(row)]
    lines = list(head if head is not None else (START, *MENU, LAUNCH)) + body
    lines += list(timing) + list(done)
    return "\n".join(f"[WARN] GBA Debug: {line}" for line in lines)


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


class HostReferenceTests(unittest.TestCase):
    """The host trace must be a complete reference without any ARM log."""

    def load(self, trace):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        path = Path(tmp.name) / "trace.json"
        path.write_text(json.dumps(trace))
        return replay.load_trace(path)

    def periodic_index(self, trace, tick):
        return next(i for i, row in enumerate(trace["events"])
                    if row["kind"] == "periodic" and row["host_tick"] == tick)

    def test_missing_periodic_observation(self):
        trace = sample(stop=3000)
        del trace["events"][self.periodic_index(trace, 2000)]
        with self.assertRaisesRegex(ValueError, r"cadence: missing \[2000\]"):
            self.load(trace)

    def test_missing_stop_tick_periodic_observation(self):
        trace = sample()
        del trace["events"][self.periodic_index(trace, 2000)]
        with self.assertRaisesRegex(ValueError, r"cadence: missing \[2000\]"):
            self.load(trace)

    def test_off_cadence_and_duplicate_periodic_observations(self):
        trace = sample()
        trace["events"].insert(self.periodic_index(trace, 1000) + 1,
                               observation("periodic", 1500, minerals=50))
        with self.assertRaisesRegex(ValueError, r"unexpected \[1500\]"):
            self.load(trace)
        trace = sample()
        i = self.periodic_index(trace, 1000)
        trace["events"].insert(i + 1, dict(trace["events"][i]))
        with self.assertRaisesRegex(ValueError, "duplicate periodic observation at tick 1000"):
            self.load(trace)

    def test_missing_or_non_integer_field(self):
        for kind, tick in (("periodic", 1000), ("won", 2000), ("start", 0)):
            trace = sample()
            row = next(r for r in trace["events"] if r["kind"] == kind and r["host_tick"] == tick)
            del row["gas"]
            with self.subTest(kind=kind), \
                    self.assertRaisesRegex(ValueError, f"{kind} observation at tick {tick} "
                                           "lacks integer fields: gas"):
                self.load(trace)
        for value in ("0", 0.0, True, None):
            trace = sample()
            trace["events"][self.periodic_index(trace, 1000)]["minerals"] = value
            with self.subTest(value=value), self.assertRaisesRegex(ValueError, "fields: minerals"):
                self.load(trace)

    def test_unknown_observation_kind(self):
        trace = sample()
        trace["events"][self.periodic_index(trace, 1000)]["kind"] = "checkpoint"
        with self.assertRaisesRegex(ValueError, "unknown kind 'checkpoint'"):
            self.load(trace)

    def test_labelled_checkpoints_are_accepted(self):
        trace = sample()
        trace["events"].insert(self.periodic_index(trace, 1000),
                               observation("wave-launch", 700, minerals=50))
        plan = self.load(trace)
        result = replay.compare(plan, arm_log(probe_rows(plan)))
        self.assertTrue(replay.verdict(plan, result))

    def test_dropped_host_periodic_cannot_hide_an_arm_mismatch(self):
        """The review's counterexample: drop the host's tick-1000 periodic
        record and give the ARM log gas=999 there."""
        trace = sample()
        plan = self.load(trace)
        rows = probe_rows(plan)
        rows[1] = dict(rows[1], gas=999)
        result = replay.compare(plan, arm_log(rows))
        self.assertIn("tick 1000 gas: ARM=999 host=0", result["mismatches"])
        self.assertFalse(replay.verdict(plan, result))
        del trace["events"][self.periodic_index(trace, 1000)]
        with self.assertRaisesRegex(ValueError, r"cadence: missing \[1000\]"):
            self.load(trace)


class CompareTests(unittest.TestCase):
    def plan(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        path = Path(tmp.name) / "trace.json"
        path.write_text(json.dumps(sample()))
        return replay.load_trace(path)

    def test_matching_log_has_no_mismatches(self):
        plan = self.plan()
        result = replay.compare(plan, arm_log(probe_rows(plan)))
        self.assertEqual(result["log_errors"], [])
        self.assertEqual(result["mismatches"], [])
        self.assertEqual(result["compared_observations"], 2)
        self.assertEqual(result["arm_final"]["kind"], "won")
        self.assertTrue(result["done"])
        self.assertEqual(result["arm_final"]["won"], 1)
        self.assertEqual(result["timing_errors"], [])
        self.assertEqual(result["guest_timing"]["mission_frames"], 2000)
        self.assertEqual(result["guest_timing"]["menu_frames"], 13)
        self.assertTrue(replay.verdict(plan, result))

    def test_field_and_missing_observations_are_reported(self):
        plan = self.plan()
        rows = probe_rows(plan)
        rows[1] = dict(rows[1], gas=7)
        result = replay.compare(plan, arm_log(rows[:2]))
        self.assertIn("tick 1000 gas: ARM=7 host=0", result["mismatches"])
        self.assertIn("missing ARM observation at tick 2000", result["mismatches"])


class TimingTests(unittest.TestCase):
    """A matching log without one valid timing record must not qualify."""

    def check_rejected(self, timing, message):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        path = Path(tmp.name) / "trace.json"
        path.write_text(json.dumps(sample()))
        plan = replay.load_trace(path)
        result = replay.compare(plan, arm_log(probe_rows(plan), timing))
        self.assertIsNone(result["guest_timing"])
        self.assertTrue(any(message in e for e in result["timing_errors"]), result["timing_errors"])
        self.assertFalse(replay.verdict(plan, result))

    def test_missing_mission_timing(self):
        self.check_rejected((MENU_TIMING,), "expected one T9 CYCLES record, found 0")

    def test_missing_menu_timing(self):
        self.check_rejected((MISSION_TIMING,), "expected one T9 MENU_CYCLES record, found 0")

    def test_duplicate_mission_timing(self):
        self.check_rejected((MENU_TIMING, MISSION_TIMING, MISSION_TIMING),
                            "expected one T9 CYCLES record, found 2")

    def test_malformed_buckets(self):
        self.check_rejected((MENU_TIMING, MISSION_TIMING.rsplit(",", 1)[0]), "malformed T9 CYCLES")

    def test_truncated_record(self):
        self.check_rejected((MENU_TIMING, "T9 CYCLES frames=2000 total=561792000"),
                            "malformed T9 CYCLES")

    def test_menu_frames_counted_as_mission(self):
        self.check_rejected((MENU_TIMING, MISSION_TIMING.replace("frames=2000", "frames=2013")
                             .replace("buckets=0,1990", "buckets=0,2003")),
                            "expected ticks 1..2000")

    def test_bucket_sum_mismatch(self):
        self.check_rejected((MENU_TIMING, MISSION_TIMING.replace("0,1990,10", "0,1990,11")),
                            "buckets sum to 2001")

    def test_inconsistent_cycles(self):
        self.check_rejected((MENU_TIMING, MISSION_TIMING.replace("update=200000000",
                                                                 "update=900000000")),
                            "inconsistent")
        self.check_rejected((MENU_TIMING, MISSION_TIMING.replace("peak=600000", "peak=0")),
                            "positive")


class LogShapeTests(unittest.TestCase):
    """Markers, field bundles and duplicates are validated, not just found."""

    def setUp(self):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        path = Path(tmp.name) / "trace.json"
        path.write_text(json.dumps(sample()))
        self.plan = replay.load_trace(path)
        self.rows = probe_rows(self.plan)

    def rejected(self, text, message):
        result = replay.compare(self.plan, text)
        self.assertTrue(any(message in e for e in result["log_errors"]), result["log_errors"])
        self.assertFalse(replay.verdict(self.plan, result))
        return result

    def test_launch_marker_without_fields(self):
        self.rejected(arm_log(self.rows, head=(START, *MENU, "T9 LAUNCH ok gibberish")),
                      "malformed probe line: 'T9 LAUNCH ok gibberish'")

    def test_done_marker_without_fields(self):
        result = self.rejected(arm_log(self.rows, done=("T9 DONE gibberish",)),
                               "malformed probe line: 'T9 DONE gibberish'")
        self.assertFalse(result["done"])

    def test_launch_fields_must_describe_a_menu_start(self):
        for bad in (LAUNCH.replace("mission=12", "mission=11"),
                    LAUNCH.replace("menu_frames=13", "menu_frames=0"),
                    LAUNCH.replace("mission_frames=1", "mission_frames=14"),
                    LAUNCH.replace("race_mode=0", "race_mode=1")):
            self.rejected(arm_log(self.rows, head=(START, *MENU, bad)), "LAUNCH fields")

    def test_done_must_consume_the_whole_tape(self):
        self.rejected(arm_log(self.rows, done=("T9 DONE cursor=3",)), "consumed 3 of 4")

    def test_start_must_match_the_trace(self):
        self.rejected(arm_log(self.rows, head=(START.replace("tape=4", "tape=5"), *MENU, LAUNCH)),
                      "START does not match")

    def test_menu_route_must_be_the_normal_one(self):
        menu = (MENU[0].replace("key=1", "key=32"),) + MENU[1:]
        self.rejected(arm_log(self.rows, head=(START, *menu, LAUNCH)), "menu keys")

    def test_failed_or_early_launch_is_rejected(self):
        self.rejected(arm_log(self.rows, head=(START, *MENU, "T9 LAUNCH fail menu_frames=13")),
                      "malformed probe line")
        self.rejected(arm_log(self.rows[:2], done=("T9 EARLY_END host_tick=1500", DONE)),
                      "out of shape")

    def test_appended_conflicting_check2_lines(self):
        text = arm_log(self.rows)
        text += "\n[WARN] GBA Debug: T9 CHECK2 host_tick=2000 gas=999"
        text += "\n[WARN] GBA Debug: T9 CHECK2 host_tick=2000 gas=0"
        self.rejected(text, "out of shape")

    def test_partial_check2_bundle(self):
        lines = arm_log(self.rows).splitlines()
        lines[-4] = "[WARN] GBA Debug: T9 CHECK2 host_tick=2000 gas=0"
        self.rejected("\n".join(lines), "malformed probe line")

    def test_duplicate_observation_cannot_repair_evidence(self):
        bad = dict(self.rows[1], gas=999)
        self.rejected(arm_log([self.rows[0], bad, self.rows[1], *self.rows[2:]]),
                      "duplicate or unordered periodic observation at tick 1000")

    def test_stop_tick_records_must_agree(self):
        rows = list(self.rows)
        rows[2] = dict(rows[2], gas=999)
        self.rejected(arm_log(rows), "conflicting observations at tick 2000")

    def test_check2_must_pair_with_its_check(self):
        lines = arm_log(self.rows).splitlines()
        i = next(i for i, l in enumerate(lines) if "T9 CHECK2 host_tick=1000 " in l)
        lines[i] = lines[i].replace("host_tick=1000", "host_tick=2000")
        self.rejected("\n".join(lines), "does not pair")

    def test_observation_ticks_follow_the_probe_schedule(self):
        extra = dict(self.rows[1], host_tick=1500)
        self.rejected(arm_log([self.rows[0], self.rows[1], extra, *self.rows[2:]]),
                      "unexpected ARM periodic observation at tick 1500")
        self.rejected(arm_log(self.rows[1:]), "missing ARM launch observation at tick 1")

    def test_other_debug_text_is_rejected(self):
        text = "[WARN] GBA Debug: stray\n" + arm_log(self.rows)
        self.rejected(text, "non-probe debug line")


if __name__ == "__main__":
    unittest.main()
