# SPDX-License-Identifier: GPL-3.0-or-later
import unittest
from tools.sc64_gas_gba_clock import summarize


def trip():
    return [(i, 100 * (i + 1), [(7, -1, 1, 9, 59-i, 0 if i < 59 else 8)])
            for i in range(60)]


class GasClockTest(unittest.TestCase):
    def test_complete_countdown(self):
        result = summarize(trip())
        self.assertEqual((result['intervals'], result['frame_spans']), (1, [59]))

    def test_discontinuity_cannot_complete_old_trip(self):
        for change in ('absent', 'owner', 'site', 'timer', 'gap', 'kind'):
            rows = trip()
            frame, cycles, workers = rows[20]
            w = list(workers[0])
            if change == 'absent': workers = []
            elif change == 'gap': rows = rows[:20] + rows[21:]; workers = None
            else:
                w[{'kind': 1, 'owner': 2, 'site': 3, 'timer': 4}[change]] += 1
                workers = [w]
            if workers is not None: rows[20] = (frame, cycles, workers)
            with self.subTest(change=change):
                self.assertEqual(summarize(rows)['intervals'], 0)

    def test_monotonicity_and_duplicate_slots(self):
        for rows in ([(1, 2, []), (1, 3, [])], [(1, 2, []), (2, 2, [])],
                     [(1, 2, [(7, -1, 1, 9, 59, 0)] * 2)]):
            with self.assertRaises(ValueError): summarize(rows)


if __name__ == '__main__':
    unittest.main()
