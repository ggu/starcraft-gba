#!/usr/bin/env python3
"""Exercise matrix isolation and aggregation without a controller binary."""
from __future__ import annotations

import contextlib
import io
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import sc64_skirmish_controller_matrix as matrix


def witness(map_id: int, race: int, count: int, outcome: str) -> dict:
    bots = [{"owner": owner, "race": (race + 1 + i) % 3}
            for i, owner in enumerate((1, 3, 4)[:count])]
    rows = [{"kind": "input", "host_tick": i, "frame": int(i >= 6),
             "held": held, "pressed": held}
            for i, held in enumerate((128, 0, 1, 0, 1, 0), 1)]
    rows += [{"kind": "start", "host_tick": 6, "frame": 1,
              "map": map_id, "human_race": race, "bots": bots},
             {"kind": "input", "host_tick": 7, "frame": 2, "held": 1, "pressed": 1}]
    won = outcome == "won"
    return {"profile": "unified-race-skirmish", "events": rows,
            "terminal": {"kind": outcome, "host_tick": 8, "frame": 3,
                         "won": int(won), "lost": int(not won), "map": map_id,
                         "human_race": race, "bot_count": count,
                         "human": {"owner": 0, "race": race, "units": 1,
                                   "buildings": int(won)},
                         "bots": [{**b, "units": 0, "buildings": int(not won)} for b in bots]}}


class MatrixTests(unittest.TestCase):
    def execute(self, fault=None):
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        root = Path(temp.name)
        controller = root / "controller"
        controller.write_text("fixture")
        output = root / "out"
        output.mkdir()
        calls = []
        def fake_run(args, **kwargs):
            map_id, race, count = map(int, args[1:4])
            path = Path(args[4])
            calls.append((map_id, race, count))
            if fault:
                result = fault(map_id, race, count, path)
                if result is not None:
                    return result
            outcome = "won" if (map_id + race + count) % 2 else "lost"
            path.write_text(json.dumps(witness(map_id, race, count, outcome)))
            return mock.Mock(returncode=0 if outcome == "won" else 3)
        return output, controller, calls, fake_run

    def test_valid_wins_and_losses_aggregate_all_cases(self):
        output, controller, calls, fake_run = self.execute()
        with mock.patch.object(matrix.subprocess, "run", side_effect=fake_run), contextlib.redirect_stdout(io.StringIO()):
            report = matrix.run(controller, output)
        self.assertEqual(len(calls), 18)
        self.assertEqual((report["wins"], report["losses"]), (9, 9))
        self.assertEqual(report["failures"], [])
        self.assertEqual(len(report["matches"]), 18)
        self.assertEqual(json.loads((output / "matrix.json").read_text()), report)

    def test_bad_witnesses_and_exits_are_recorded_then_matrix_continues(self):
        def faults(m, r, n, path):
            if (m, r, n) == (0, 0, 1):
                path.write_text('{"profile":')  # malformed JSON
            elif (m, r, n) == (0, 0, 2):
                bad = witness(m, r, n, "won")
                bad["events"][6]["bots"][0] = 42  # malformed nested type
                path.write_text(json.dumps(bad))
            elif (m, r, n) == (0, 0, 3):
                return mock.Mock(returncode=0)  # missing fresh trace
            elif (m, r, n) == (0, 1, 1):
                path.write_text(json.dumps(witness(m, r, n, "lost")))
                return mock.Mock(returncode=0)  # exit disagrees with valid witness
            elif (m, r, n) == (0, 1, 2):
                path.write_text(json.dumps(witness(1, r, n, "won")))
                return mock.Mock(returncode=0)  # valid witness, wrong requested configuration
            elif (m, r, n) == (0, 1, 3):
                return mock.Mock(returncode=9)  # old trace must never be read
            else:
                return None
            return mock.Mock(returncode=0)
        output, controller, calls, fake_run = self.execute(faults)
        for label in ("m0-h0-n3", "m0-h1-n3"):
            (output / f"{label}.json").write_text(json.dumps(witness(0, 0, 1, "won")))
        with mock.patch.object(matrix.subprocess, "run", side_effect=fake_run), contextlib.redirect_stdout(io.StringIO()):
            with self.assertRaisesRegex(RuntimeError, "6 matches incomplete"):
                matrix.run(controller, output)
        report = json.loads((output / "matrix.json").read_text())
        self.assertEqual(len(calls), 18)
        self.assertEqual(calls[-1], (1, 2, 3))
        self.assertEqual([f["configuration"] for f in report["failures"]],
                         ["m0-h0-n1", "m0-h0-n2", "m0-h0-n3", "m0-h1-n1", "m0-h1-n2", "m0-h1-n3"])
        self.assertEqual(len(report["matches"]), 12)
        self.assertIn("exit code and terminal witness disagree", report["failures"][3]["error"])
        self.assertIn("different configuration", report["failures"][4]["error"])
        self.assertIn("controller exit 9", report["failures"][5]["error"])
        self.assertFalse((output / "m0-h0-n3.json").exists())
        self.assertFalse((output / "m0-h1-n3.json").exists())

    def test_every_case_failing_still_writes_complete_report(self):
        def invalid(_m, _r, _n, path):
            path.write_text("[]")
            return mock.Mock(returncode=0)
        output, controller, calls, fake_run = self.execute(invalid)
        with mock.patch.object(matrix.subprocess, "run", side_effect=fake_run), contextlib.redirect_stdout(io.StringIO()):
            with self.assertRaisesRegex(RuntimeError, "18 matches incomplete"):
                matrix.run(controller, output)
        report = json.loads((output / "matrix.json").read_text())
        self.assertEqual(len(calls), 18)
        self.assertEqual(len(report["failures"]), 18)
        self.assertEqual(report["matches"], [])
        self.assertEqual((report["wins"], report["losses"]), (0, 0))


if __name__ == "__main__":
    unittest.main()
