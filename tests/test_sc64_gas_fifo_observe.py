# SPDX-License-Identifier: GPL-3.0-or-later
"""Executable constraints for a future gas-service observation adapter."""
import unittest
from tools.sc64_gas_fifo_observe import analyze


class GasFifoObservation(unittest.TestCase):
    def test_oldest_and_independent_sites(self):
        rows = [(0, 0, [(1, 82, 'a'), (2, 82, 'b')]),
                (1, 1, [(1, 82, 'a'), (2, 82, 'b'), (3, 82, 'a')]),
                (2, 2, [(1, 83, 'a'), (2, 83, 'b'), (3, 82, 'a')]),
                (3, 3, [(3, 83, 'a')])]
        self.assertEqual(analyze(rows)['grants'], [2, 2, 3])

    def test_younger_waiter_and_duplicate_active_are_rejected(self):
        with self.assertRaisesRegex(ValueError, 'older waiter'):
            analyze([(0, 0, [(1, 82, 0)]), (1, 1, [(1, 82, 0), (2, 82, 0)]),
                     (2, 2, [(1, 82, 0), (2, 83, 0)])])
        with self.assertRaisesRegex(ValueError, 'multiple active'):
            analyze([(0, 0, [(1, 83, 0), (2, 83, 0)])])

    def test_equal_first_sample_does_not_impose_tie_break(self):
        self.assertEqual(analyze([(0, 0, [(1, 82, 0), (2, 82, 0)]),
                                  (1, 1, [(1, 82, 0), (2, 83, 0)])])['grants'], [1])

    def test_cancelled_waiter_no_longer_blocks(self):
        rows = [(0, 0, [(1, 82, 0)]), (1, 1, [(1, 82, 0), (2, 82, 0)]),
                (2, 2, [(2, 82, 0)]), (3, 3, [(2, 83, 0)])]
        self.assertEqual(analyze(rows)["grants"], [3])

    def test_unobserved_wait_is_not_counted_as_proof(self):
        with self.assertRaisesRegex(ValueError, 'without observed wait'):
            analyze([(0, 0, [(1, 83, 0)])])


if __name__ == '__main__':
    unittest.main()
