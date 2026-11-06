# SPDX-License-Identifier: GPL-3.0-or-later
import copy
import importlib.util
from pathlib import Path
import unittest

spec = importlib.util.spec_from_file_location("egg_tape", Path(__file__).resolve().parents[1] / "tools/sc64_egg_custom_tape.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


def tape():
    return {"profile": "z7-custom-egg", "events": [
        {"kind": "input", "host_tick": 1, "held": 1, "pressed": 1},
        {"kind": "input", "host_tick": 2, "held": 0, "pressed": 0},
        {"kind": "start", "host_tick": 2, "human_race": 1, "bots": [{}]},
        {"kind": "paid_egg", "host_tick": 3, "slot": 4, "minerals_before": 100,
         "minerals_after": 50, "larva_before": 2, "larva_after": 1, "zerglings_before": 0},
        {"kind": "save", "host_tick": 4, "slot": 4, "timer": 10},
        {"kind": "birth", "host_tick": 5, "zerglings": 2},
    ]}


class TapeTest(unittest.TestCase):
    def test_valid_tick_translation(self):
        self.assertEqual(module.validate(tape())["transitions"], [(0, 1), (1, 0)])

    def test_reject_bad_checkpoint_and_edges(self):
        for row, key, value in [(0, "pressed", 0), (1, "host_tick", 1),
                                (3, "minerals_after", 51), (4, "slot", 5),
                                (4, "timer", 0), (5, "zerglings", 1)]:
            d = copy.deepcopy(tape())
            d["events"][row][key] = value
            with self.subTest(row=row, key=key), self.assertRaises(ValueError):
                module.validate(d)


if __name__ == "__main__":
    unittest.main()
