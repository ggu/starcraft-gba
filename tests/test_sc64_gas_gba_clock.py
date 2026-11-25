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

    def test_stacked_timers_and_independent_sites(self):
        def workers(a, b, c):
            return [(0, 7, 0, 9, a, 0), (1, 7, 0, 9, b, 0), (2, 7, 0, 15, c, 0)]
        rows = [(1, 1, workers(12, 15, 0)), (2, 2, workers(11, 15, 59))]
        result = summarize(rows)
        self.assertEqual(result['peak_countdowns'], {9: 2, 15: 1})
        self.assertEqual(result['peak_advancing'], {9: 1, 15: 1})
        self.assertEqual(result['parallel_site_frames'], 1)
        rows.append((3, 3, workers(10, 14, 58)))
        self.assertEqual(summarize(rows)['peak_advancing'][9], 2)
        self.assertFalse(summarize([rows[0], rows[2]])['peak_advancing'])

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
