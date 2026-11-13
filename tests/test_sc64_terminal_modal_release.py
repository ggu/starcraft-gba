# SPDX-License-Identifier: GPL-3.0-or-later
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from sc64_z2_normal_replay import terminal_modal_release


class TerminalModalReleaseTest(unittest.TestCase):
    def test_only_authenticated_release_before_terminal_neutral(self):
        keys = {17: 385, 18: 384, 19: 0}
        checks = {
            17: [("modal", 15, 4, 1, 5, 1, 384, 1)],
            18: [("armed", 15, 4, 1, 1)],
            19: [("released", 15)],
        }
        final = {"frame": 15, "won": 1, "lost": 0}
        args = [19, 20, keys, checks, (0, 0, 0, 0), 15, final]
        self.assertTrue(terminal_modal_release(*args))
        for index, wrong in (
            (0, 18), (1, 21), (2, keys | {19: 1}), (3, {}),
            (4, (1, 0, 0, 4)), (5, 16), (6, final | {"lost": 1}),
        ):
            with self.subTest(index=index):
                altered = args.copy()
                altered[index] = wrong
                self.assertFalse(terminal_modal_release(*altered))
