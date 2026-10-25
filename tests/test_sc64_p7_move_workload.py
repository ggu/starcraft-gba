"""Synthetic checks for the P7 workload runner; no ROM or emulator required."""
import argparse
import contextlib
import importlib.util
import io
import json
from pathlib import Path
import sys
import tempfile
import unittest

TOOLS = Path(__file__).resolve().parents[1] / 'tools'
sys.path.insert(0, str(TOOLS))
spec = importlib.util.spec_from_file_location('sc64_p7_move_workload', TOOLS / 'sc64_p7_move_workload.py')
workload = importlib.util.module_from_spec(spec)
spec.loader.exec_module(workload)


def result(fps, hp=100, held=0):
    point = {'label': 'neutral+100', 'frame': 102, 'unit_hp_by_owner': {'0': hp},
             'effective_fps': fps, 'elapsed_hardware_cycles': 1, 'routes': {},
             'profile_cycles_last_tick': {'sc64_orders_cycles': fps}}
    return {'checkpoints': [point], 'inputs': [{'demo_frame': 1, 'held': held}],
            'command': {}, 'ended': {'screen': 1}, 'neutral_fps': fps, 'attack_fps': None}


class P7MoveWorkloadTests(unittest.TestCase):
    def test_tick_bounds(self):
        self.assertEqual(workload.ticks_arg('0'), 0)
        self.assertEqual(workload.ticks_arg('2400'), 2400)
        for bad in ('-100', '150', str(workload.MAX_TICKS + 100)):
            with self.assertRaises(argparse.ArgumentTypeError):
                workload.ticks_arg(bad)

    def compare(self, reference, candidate):
        with tempfile.TemporaryDirectory() as directory:
            paths = [Path(directory) / name for name in ('a.json', 'b.json')]
            for path, data in zip(paths, (reference, candidate)):
                path.write_text(json.dumps(data))
            with contextlib.redirect_stdout(io.StringIO()):
                return workload.compare(*paths)

    def test_compare_ignores_timing_only(self):
        self.assertEqual(self.compare(result(10.0), result(12.0)), 0)

    def test_compare_rejects_state_or_input_differences(self):
        self.assertEqual(self.compare(result(10.0), result(10.0, hp=99)), 1)
        self.assertEqual(self.compare(result(10.0), result(10.0, held=1)), 1)


if __name__ == '__main__':
    unittest.main()
