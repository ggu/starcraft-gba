import json
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
sys.path.insert(0, str(ROOT / "tests"))

import sc64_z7_trace_parity as parity
import test_sc64_z7_controller_replay as controller_fixture


def trace_bytes(kind="won"):
    trace = controller_fixture.z7_trace(kind)
    final = trace["events"][-1]
    final.update(minerals=321, gas=78)
    return json.dumps(trace).encode("utf-8")


def output_row(tick, *, final=False, outcome=1):
    row = [str(tick), f"{tick:016x}", "389" if final else "1",
           "3" if final else "1", "0", "1" if final and outcome == 1 else "0",
           "1" if final and outcome == 2 else "0", str(outcome if final else 0),
           "6" if final and outcome == 1 else "3" if final and outcome == 2 else "2",
           "1", "0", "0", "0", "0", "321", "78", "0", "0",
           "0,0,0,0,0,0,0,0", "3"]
    return (" ".join(row) + "\n").encode("ascii")


class Z7TraceParityTests(unittest.TestCase):
    def test_authenticates_win_and_failed_attempt_and_encodes_sparse_inputs(self):
        with tempfile.TemporaryDirectory() as tmp:
            win = Path(tmp) / "win.json"
            win.write_bytes(trace_bytes("won"))
            win_plan, win_tape = parity.validate_trace_file(win)
            failed = Path(tmp) / "failed.json"
            failed.write_bytes(trace_bytes("lost"))
            failed_plan, failed_tape = parity.validate_trace_file(failed)
        self.assertEqual(win_plan["final"]["kind"], "won")
        self.assertEqual(failed_plan["final"]["kind"], "lost")
        self.assertIn(b"400 ", win_tape.splitlines()[0])
        self.assertEqual(win_tape.splitlines()[1:], failed_tape.splitlines()[1:])
        self.assertGreater(win_plan["input_count"], 0)

    def test_rejects_wrong_profile_without_an_unauthenticated_mode(self):
        trace = json.loads(trace_bytes())
        trace["profile"] = "zerg-z6"
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "bad.json"
            path.write_text(json.dumps(trace), encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "profile"):
                parity.validate_trace_file(path)

    def test_rejects_nonobject_terminal_and_truncated_streams(self):
        trace = json.loads(trace_bytes())
        trace["events"][-1] = "invalid terminal"
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "invalid.json"
            path.write_text(json.dumps(trace), encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "must be an object"):
                parity.validate_trace_file(path)
            left, right = Path(tmp) / "left", Path(tmp) / "right"
            left.write_bytes(b"")
            right.write_bytes(b"")
            with self.assertRaisesRegex(RuntimeError, "observed 0"):
                parity._compare_streams(left, right, 1)
            left.write_bytes(output_row(1))
            with self.assertRaisesRegex(RuntimeError, "row-count mismatch at update 1"):
                parity._compare_streams(left, right, 1)

    def test_optimization_variants_preserve_separate_evidence_files(self):
        root, trace = Path("private-output"), Path("same-trace.json")
        count = parity._output_paths(root, 1, trace, "count-snapshot")
        sight = parity._output_paths(root, 1, trace, "sight-spans")
        defense = parity._output_paths(root, 1, trace, "defense-targets")
        self.assertTrue(set(count).isdisjoint(sight))
        self.assertTrue(set(count).isdisjoint(defense))
        self.assertTrue(set(sight).isdisjoint(defense))
        self.assertEqual(parity.VARIANTS["defense-targets"],
                         ("build/test-sc64-z7-trace-sight-spans",
                          "build/test-sc64-z7-trace-defense-targets"))
        with self.assertRaisesRegex(ValueError, "unknown optimization"):
            parity.run([trace], root, "unrecognized")

    def test_checks_every_row_and_reports_first_state_difference(self):
        plan = {"stop_tick": 400, "final": {"kind": "won", "frame": 389,
                "screen": 3, "outcome": 1, "started": 6,
                "minerals": 321, "gas": 78}}
        with tempfile.TemporaryDirectory() as tmp:
            left, right = Path(tmp) / "left", Path(tmp) / "right"
            rows = [output_row(tick, final=tick == 400) for tick in range(1, 401)]
            left.write_bytes(b"".join(rows))
            right.write_bytes(b"".join(rows))
            self.assertEqual(parity._validate_rows(left, plan)[0], 400)
            parity._compare_streams(left, right, 400)
            rows[250] = output_row(251, final=False)[:-1] + b"X\n"
            right.write_bytes(b"".join(rows))
            with self.assertRaisesRegex(RuntimeError, "update 251"):
                parity._compare_streams(left, right, 400)


if __name__ == "__main__":
    unittest.main()
