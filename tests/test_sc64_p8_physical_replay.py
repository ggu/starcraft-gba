# SPDX-License-Identifier: GPL-3.0-or-later
"""Pure physical-input timeline checks; no emulator or guest writes."""
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import sc64_p8_physical_replay as p8


class TimelineTests(unittest.TestCase):
    def test_pause_save_and_fresh_load_alignment(self):
        cut = 32443
        original = [(32439,8),(32440,0),(32441,2),(32442,0),
                    (32443,8),(32444,0),(32445,400)]
        events = p8.resume_events(original,cut)
        self.assertEqual(events, [(0,512),(1,0),(2,400)])
        self.assertEqual(p8.resume_boundary(32445,cut),2)
        # play_to(end) applies events at indices strictly below end. Loading
        # replaces START at index 0; release at index 1 advances the world.
        frame = 31676
        for index in range(p8.resume_boundary(32445,cut)):
            held = dict(events).get(index,0)
            if index == 0:
                self.assertEqual(held,512)
            else:
                self.assertEqual(held,0)
                frame += 1
        self.assertEqual(frame,31677)

    def test_invalid_boundary(self):
        with self.assertRaises(ValueError): p8.resume_boundary(32443,32443)

if __name__ == '__main__': unittest.main()
