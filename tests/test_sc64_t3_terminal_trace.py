"""The countdown replay accepts older traces and validates full terminal observations."""
import json
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import sc64_t9_normal_replay as replay


class CountdownTrace(unittest.TestCase):
    def setUp(self):
        keys = (replay.KEY_A, 0, replay.KEY_RIGHT, 0, replay.KEY_A, 0, replay.KEY_START, 0)
        self.trace = dict(trace_schema=1, mode="controller-only-neutral-run",
                          mission=6, campaign_mission=6, selected_mission=6,
                          total_ticks=107, elapsed_ticks=100, remaining_ticks=0,
                          events=[dict(tick=i, held=held) for i, held in enumerate(keys)],
                          checkpoints=[], outcome=1, objective=0, buildings_killed=0,
                          won=1, lost=0, initial_units=138, initial_buildings=31)
        self.terminal = dict.fromkeys(replay.T3_FIELDS, 0)
        self.terminal.update(tick=107, elapsed=100, outcome=1, phase=3, won=1,
                             screen=3, side_ore=75, waves=11)

    def load(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "trace.json"
            path.write_text(json.dumps(self.trace))
            return replay.load_t3_trace(path, 100)["observations"][100]

    def test_legacy_and_complete_terminal_rows(self):
        self.assertNotIn("side_ore", self.load())
        self.trace["terminal_checkpoint"] = self.terminal
        result = self.load()
        self.assertEqual(result["kind"], "won")
        for name in replay.T3_FIELDS:
            self.assertEqual(result[name], self.terminal[name])

    def test_incomplete_or_inconsistent_terminal_rows_fail(self):
        for key, value in (("tick", 106), ("elapsed", 99), ("outcome", 0),
                           ("side_ore", None), ("waves", False)):
            with self.subTest(key=key):
                self.trace["terminal_checkpoint"] = dict(self.terminal, **{key: value})
                with self.assertRaises(ValueError):
                    self.load()
